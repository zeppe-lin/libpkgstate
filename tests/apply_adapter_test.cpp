// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "test.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <libpkgapply/object_fact.h>
#include <libpkgapply/path_consequence.h>
#include <libpkgapply/request.h>
#include <libpkgapply/result.h>
#include <libpkgapply/state_projection.h>
#include <libpkgapply/target_context.h>
#include <libpkgapply/execution_control.h>
#include <libpkgimage/inspection_receipt.h>
#include <libpkgimage/package_entry.h>
#include <libpkgimage/package_image.h>
#include <libpkgplan/install.h>
#include <libpkgplan/remove.h>
#include <libpkgplan/upgrade.h>
#include <libpkgstate-apply/adapter.h>
#include <libpkgstate/installed_control.h>
#include <libpkgstate/installed_package.h>
#include <libpkgstate/package_release.h>
#include <libpkgstate/snapshot.h>
#include <libpkgstate/state_target_binding.h>

namespace {

template<typename Identity>
Identity
state_identity(std::uint8_t value)
{
  pkgstate::sha256_digest_bytes bytes{};
  bytes.fill(value);
  return Identity::from_sha256(bytes);
}

template<typename Identity>
Identity
plan_identity(std::uint8_t value)
{
  pkgplan::sha256_digest_bytes bytes{};
  bytes.fill(value);
  return Identity::from_sha256(bytes);
}

template<typename Identity>
Identity
apply_identity(std::uint8_t value)
{
  std::string text = "v1:sha256:";
  constexpr char hex[] = "0123456789abcdef";
  for (std::size_t index = 0; index < 32; ++index)
  {
    const std::uint8_t byte =
        static_cast<std::uint8_t>(value + index);
    text.push_back(hex[(byte >> 4) & 0x0f]);
    text.push_back(hex[byte & 0x0f]);
  }
  return Identity::parse(text);
}

template<typename Destination, typename Source>
Destination
translate_identity(const Source& source)
{
  return Destination::parse(source.string());
}

pkgstate::state_target_binding
state_target(std::uint8_t seed = 1)
{
  return pkgstate::state_target_binding::make(
      state_identity<pkgstate::managed_target_identity>(seed),
      state_identity<pkgstate::state_store_identity>(seed + 1),
      state_identity<pkgstate::root_view_identity>(seed + 2),
      state_identity<pkgstate::state_backend_identity>(seed + 3),
      state_identity<pkgstate::publication_domain_identity>(seed + 4));
}

pkgstate::installed_package
state_package(const pkgstate::state_target_binding& target,
              const char* version,
              std::vector<pkgstate::owned_entry> manifest)
{
  const pkgstate::package_release release =
      pkgstate::package_release::make("tool", version, "1");
  pkgstate::installed_control_completeness completeness;
  const pkgstate::installed_control control =
      pkgstate::installed_control::make(
          release,
          completeness,
          {pkgstate::runtime_dependency_declaration::make("libc >= 0")},
          {pkgstate::removal_lifecycle_declaration::make(
              pkgstate::removal_lifecycle_phase::post_remove,
              "application/x-zeppe-lin-shell",
              "finish-old-remove")},
          {pkgstate::target_profile_fact::make("architecture", "x86_64")},
          {pkgstate::control_provenance::make(
              pkgstate::control_provenance_kind::application_evidence,
              state_identity<pkgstate::application_evidence_identity>(90)
                  .string())});
  return pkgstate::installed_package::make(
      release, control, target, std::move(manifest));
}

pkgplan::filesystem_object_metadata
planner_regular(std::uint8_t content)
{
  return pkgplan::filesystem_object_metadata(
      pkgplan::filesystem_object_kind::regular,
      0755,
      0,
      0,
      4,
      pkgplan::object_timestamp(10, 0),
      plan_identity<pkgplan::filesystem_regular_content_identity>(content));
}

pkgplan::candidate_control_projection
incoming_control()
{
  return pkgplan::candidate_control_projection(
      {pkgplan::runtime_dependency_declaration::make("libc >= 1")},
      {pkgplan::removal_lifecycle_declaration::make(
          pkgplan::removal_lifecycle_phase::pre_remove,
          "application/x-zeppe-lin-shell",
          "prepare-new-remove")},
      {pkgplan::target_profile_fact::make("architecture", "x86_64")});
}

pkgplan::installed_control_projection
planner_control(const pkgstate::installed_control& source)
{
  pkgplan::installed_control_completeness completeness;
  completeness.runtime_dependencies =
      pkgplan::control_fact_availability::known;
  completeness.removal_lifecycle =
      pkgplan::control_fact_availability::known;
  completeness.target_profile =
      pkgplan::control_fact_availability::known;

  std::vector<pkgplan::runtime_dependency_declaration> dependencies;
  for (const auto& item : source.runtime_dependencies())
    dependencies.push_back(
        pkgplan::runtime_dependency_declaration::make(item.expression()));

  std::vector<pkgplan::removal_lifecycle_declaration> lifecycle;
  for (const auto& item : source.removal_lifecycle())
  {
    lifecycle.push_back(pkgplan::removal_lifecycle_declaration::make(
        item.phase() == pkgstate::removal_lifecycle_phase::pre_remove
            ? pkgplan::removal_lifecycle_phase::pre_remove
            : pkgplan::removal_lifecycle_phase::post_remove,
        item.format(),
        item.material()));
  }

  std::vector<pkgplan::target_profile_fact> profile;
  for (const auto& item : source.target_profile())
    profile.push_back(
        pkgplan::target_profile_fact::make(item.name(), item.value()));

  return pkgplan::installed_control_projection(
      completeness,
      std::move(dependencies),
      std::move(lifecycle),
      std::move(profile));
}

pkgimage::inspected_package_image
incoming_image(std::uint8_t content)
{
  pkgimage::package_entry entry(
      pkgimage::package_path::parse("tool"),
      pkgimage::entry_type::regular);
  entry.mode = 0755;
  entry.uid = 0;
  entry.gid = 0;
  entry.size = 4;
  pkgimage::sha256_digest_bytes content_bytes{};
  content_bytes.fill(content);
  entry.regular_content =
      pkgimage::regular_content_digest::from_sha256(content_bytes);

  pkgimage::package_image image({entry});
  pkgimage::sha256_digest_bytes archive_bytes{};
  archive_bytes.fill(static_cast<std::uint8_t>(content + 30));
  pkgimage::archive_inspection_receipt receipt(
      pkgimage::archive_backend_identity::parse("test/pkgstate-apply-v1"),
      pkgimage::complete_archive_digest::from_sha256(archive_bytes),
      image.identity(),
      image.size());
  return pkgimage::inspected_package_image(
      std::move(image), std::move(receipt));
}

pkgplan::package_policy_snapshot
policy()
{
  return pkgplan::package_policy_snapshot(
      plan_identity<pkgplan::policy_snapshot_identity>(50),
      pkgplan::normalized_path_policy(
          pkgplan::incoming_path_policy::activate(),
          pkgplan::obsolete_path_policy::remove(),
          pkgplan::shared_ownership_policy::forbid,
          pkgplan::directory_cleanup_policy::remove_if_empty),
      {});
}

struct planner_context final {
  pkgplan::target_system_context_identity target =
      plan_identity<pkgplan::target_system_context_identity>(60);
  pkgplan::observation_set_identity observations =
      plan_identity<pkgplan::observation_set_identity>(61);
  pkgplan::runtime_dependency_closure_identity runtime_closure =
      plan_identity<pkgplan::runtime_dependency_closure_identity>(62);
};

pkgplan::installation_plan
installation_plan(const pkgstate::snapshot& expected,
                  const planner_context& context)
{
  const pkgplan::package_path path = pkgplan::package_path::parse("tool");
  const pkgplan::package_release release(
      plan_identity<pkgplan::package_release_identity>(70),
      "tool", "1.0", "1");
  pkgimage::inspected_package_image image = incoming_image(1);
  const auto archive = image.receipt().archive_digest();

  pkgplan::installation_request request(
      pkgplan::candidate_package_fact(
          plan_identity<pkgplan::candidate_control_identity>(71),
          release,
          incoming_control()),
      pkgplan::artifact_package_fact(
          plan_identity<pkgplan::artifact_identity>(72),
          plan_identity<pkgplan::artifact_manifest_identity>(73),
          release),
      archive,
      std::move(image),
      translate_identity<pkgplan::installed_state_snapshot_identity>(
          expected.identity()),
      pkgplan::installed_ownership_inventory(
          translate_identity<pkgplan::ownership_inventory_identity>(
              expected.ownership_identity()),
          translate_identity<pkgplan::installed_state_snapshot_identity>(
              expected.identity()),
          pkgplan::fact_set_completeness::complete,
          {}),
      context.target,
      pkgplan::target_observation_set(
          context.observations,
          context.target,
          pkgplan::fact_set_completeness::complete,
          {pkgplan::target_path_observation::absent(path)}),
      context.runtime_closure,
      policy());

  const pkgplan::installation_result result = pkgplan::plan_install(request);
  CHECK(result.has_plan());
  CHECK(result.plan() != nullptr);
  return *result.plan();
}

pkgplan::installed_package_fact
planner_installed(const pkgstate::snapshot& expected,
                  const pkgstate::installed_package& installed,
                  std::optional<pkgplan::installed_control_identity>
                      control_override = std::nullopt)
{
  const pkgstate::package_release& release = installed.release();
  return pkgplan::installed_package_fact(
      translate_identity<pkgplan::installed_package_identity>(
          installed.identity()),
      control_override.value_or(
          translate_identity<pkgplan::installed_control_identity>(
              installed.control().identity())),
      translate_identity<pkgplan::installed_state_snapshot_identity>(
          expected.identity()),
      pkgplan::package_release(
          translate_identity<pkgplan::package_release_identity>(
              release.identity()),
          release.name(), release.version(), release.release()),
      planner_control(installed.control()));
}

pkgplan::installed_ownership_inventory
planner_ownership(const pkgstate::snapshot& expected,
                  const pkgstate::installed_package& installed)
{
  const pkgplan::package_path path = pkgplan::package_path::parse("tool");
  return pkgplan::installed_ownership_inventory(
      translate_identity<pkgplan::ownership_inventory_identity>(
          expected.ownership_identity()),
      translate_identity<pkgplan::installed_state_snapshot_identity>(
          expected.identity()),
      pkgplan::fact_set_completeness::complete,
      {pkgplan::installed_ownership_claim(
          path,
          translate_identity<pkgplan::installed_package_identity>(
              installed.identity()),
          planner_regular(2))});
}

pkgplan::upgrade_plan
upgrade_plan(const pkgstate::snapshot& expected,
             const pkgstate::installed_package& installed,
             const planner_context& context,
             std::optional<pkgplan::installed_control_identity>
                 control_override = std::nullopt)
{
  const pkgplan::package_path path = pkgplan::package_path::parse("tool");
  const pkgplan::package_release release(
      plan_identity<pkgplan::package_release_identity>(74),
      "tool", "2.0", "1");
  pkgimage::inspected_package_image image = incoming_image(3);
  const auto archive = image.receipt().archive_digest();

  pkgplan::upgrade_request request(
      planner_installed(expected, installed, control_override),
      pkgplan::candidate_package_fact(
          plan_identity<pkgplan::candidate_control_identity>(75),
          release,
          incoming_control()),
      pkgplan::artifact_package_fact(
          plan_identity<pkgplan::artifact_identity>(76),
          plan_identity<pkgplan::artifact_manifest_identity>(77),
          release),
      archive,
      std::move(image),
      translate_identity<pkgplan::installed_state_snapshot_identity>(
          expected.identity()),
      planner_ownership(expected, installed),
      context.target,
      pkgplan::target_observation_set(
          context.observations,
          context.target,
          pkgplan::fact_set_completeness::complete,
          {pkgplan::target_path_observation::present(
              pkgplan::filesystem_object_fact(path, planner_regular(2)))}),
      context.runtime_closure,
      policy());

  const pkgplan::upgrade_result result = pkgplan::plan_upgrade(request);
  CHECK(result.has_plan());
  CHECK(result.plan() != nullptr);
  return *result.plan();
}

pkgplan::removal_plan
removal_plan(const pkgstate::snapshot& expected,
             const pkgstate::installed_package& installed,
             const planner_context& context)
{
  const pkgplan::package_path path = pkgplan::package_path::parse("tool");
  pkgplan::removal_request request(
      planner_installed(expected, installed),
      translate_identity<pkgplan::installed_state_snapshot_identity>(
          expected.identity()),
      planner_ownership(expected, installed),
      context.target,
      pkgplan::target_observation_set(
          context.observations,
          context.target,
          pkgplan::fact_set_completeness::complete,
          {pkgplan::target_path_observation::present(
              pkgplan::filesystem_object_fact(path, planner_regular(2)))}),
      policy());

  const pkgplan::removal_result result = pkgplan::plan_removal(request);
  CHECK(result.has_plan());
  CHECK(result.plan() != nullptr);
  return *result.plan();
}

pkgapply::application_target_context
application_target(const pkgstate::state_target_binding& state_target,
                   const planner_context& planner,
                   std::uint8_t seed = 1)
{
  return pkgapply::application_target_context::make(
      planner.target,
      translate_identity<pkgapply::managed_target_identity>(
          state_target.managed_target()),
      translate_identity<pkgapply::root_view_identity>(
          state_target.root_view()),
      apply_identity<pkgapply::observation_backend_identity>(seed + 10),
      apply_identity<pkgapply::mutation_backend_identity>(seed + 11),
      apply_identity<pkgapply::mutation_exclusion_domain_identity>(seed + 12),
      apply_identity<pkgapply::active_object_namespace_identity>(seed + 13),
      apply_identity<pkgapply::rejected_object_store_identity>(seed + 14),
      apply_identity<pkgapply::staging_namespace_identity>(seed + 15),
      apply_identity<pkgapply::journal_namespace_identity>(seed + 16),
      apply_identity<pkgapply::execution_capability_profile_identity>(seed + 17));
}

pkgapply::application_execution_control
execution_control(pkgapply::application_durability_requirement durability =
                      pkgapply::application_durability_requirement::
                          all_application_domains)
{
  return pkgapply::application_execution_control::make(
      pkgapply::application_recovery_requirement::best_effort,
      durability,
      pkgapply::application_cancellation_policy::recover_after_target_mutation);
}

pkgapply::application_durability_profile
durability()
{
  using D = pkgapply::application_durability_domain;
  using S = pkgapply::application_durability_status;
  return pkgapply::application_durability_profile({
      {D::journal, S::confirmed},
      {D::incoming_staging, S::confirmed},
      {D::recovery_staging, S::confirmed},
      {D::active_namespace, S::confirmed},
      {D::rejected_object_store, S::confirmed},
      {D::completed_evidence, S::confirmed},
  });
}

pkgapply::completed_object_fact
completed_regular(const pkgplan::package_path& path, std::uint8_t content)
{
  return pkgapply::completed_object_fact(
      path,
      pkgapply::completed_object_kind::regular,
      pkgapply::qualified_fact<std::uint32_t>::known(0755),
      pkgapply::qualified_fact<std::uint64_t>::known(0),
      pkgapply::qualified_fact<std::uint64_t>::known(0),
      pkgapply::qualified_fact<std::uint64_t>::known(4),
      pkgapply::qualified_fact<pkgapply::completed_object_timestamp>::known(
          {10, 0}),
      pkgapply::qualified_fact<pkgapply::completed_regular_content_identity>::
          known(apply_identity<pkgapply::completed_regular_content_identity>(
              content)),
      pkgapply::qualified_fact<std::string>::not_applicable(),
      pkgapply::qualified_fact<pkgapply::completed_device_number>::
          not_applicable(),
      pkgapply::qualified_fact<pkgapply::completed_hardlink_relation>::
          unknown(),
      pkgapply::object_fact_provenance::application_observation,
      pkgapply::object_fact_completeness::complete);
}

std::vector<pkgplan::installed_package_identity>
planner_owners(const pkgstate::snapshot& expected,
               const pkgplan::package_path& path)
{
  std::vector<pkgplan::installed_package_identity> owners;
  const pkgstate::package_path state_path =
      pkgstate::package_path::parse(path.string());
  for (const pkgstate::installed_package* owner : expected.owners(state_path))
  {
    owners.push_back(
        translate_identity<pkgplan::installed_package_identity>(
            owner->identity()));
  }
  return owners;
}

template<typename Plan>
pkgapply::lease_bound_state_projection
application_projection(
    const pkgstate::snapshot& expected,
    const Plan& plan,
    std::uint8_t seed = 30,
    bool wrong_owners = false,
    pkgapply::state_projection_completeness completeness =
        pkgapply::state_projection_completeness::complete)
{
  std::vector<pkgapply::projected_path_owners> paths;
  for (const auto& precondition : plan.preconditions().paths())
  {
    auto owners = planner_owners(expected, precondition.path());
    if (wrong_owners)
      owners.push_back(
          plan_identity<pkgplan::installed_package_identity>(99));
    paths.emplace_back(precondition.path(), std::move(owners));
  }

  return pkgapply::lease_bound_state_projection::make(
      apply_identity<pkgapply::mutation_lease_instance_identity>(seed),
      plan.preconditions().installed_snapshot(),
      plan.preconditions().ownership_inventory(),
      completeness,
      std::move(paths),
      apply_identity<pkgapply::state_projection_evidence_identity>(seed + 1));
}

pkgapply::completed_object_fact
completed_directory(const pkgplan::package_path& path)
{
  return pkgapply::completed_object_fact(
      path,
      pkgapply::completed_object_kind::directory,
      pkgapply::qualified_fact<std::uint32_t>::known(0755),
      pkgapply::qualified_fact<std::uint64_t>::known(0),
      pkgapply::qualified_fact<std::uint64_t>::known(0),
      pkgapply::qualified_fact<std::uint64_t>::not_applicable(),
      pkgapply::qualified_fact<pkgapply::completed_object_timestamp>::known(
          {10, 0}),
      pkgapply::qualified_fact<pkgapply::completed_regular_content_identity>::
          not_applicable(),
      pkgapply::qualified_fact<std::string>::not_applicable(),
      pkgapply::qualified_fact<pkgapply::completed_device_number>::
          not_applicable(),
      pkgapply::qualified_fact<pkgapply::completed_hardlink_relation>::
          not_applicable(),
      pkgapply::object_fact_provenance::application_observation,
      pkgapply::object_fact_completeness::complete);
}

pkgapply::application_path_role
application_role(pkgplan::installation_path_role role)
{
  return role == pkgplan::installation_path_role::incoming_entry
      ? pkgapply::application_path_role::incoming_entry
      : pkgapply::application_path_role::structural_parent;
}

pkgapply::application_path_role
application_role(pkgplan::upgrade_path_role role)
{
  switch (role)
  {
    case pkgplan::upgrade_path_role::incoming_entry:
      return pkgapply::application_path_role::incoming_entry;
    case pkgplan::upgrade_path_role::obsolete_old_path:
      return pkgapply::application_path_role::obsolete_old_path;
    case pkgplan::upgrade_path_role::structural_parent:
      return pkgapply::application_path_role::structural_parent;
  }
  return pkgapply::application_path_role::incoming_entry;
}

pkgapply::application_path_consequence
installation_consequence(const pkgplan::installation_path_decision& decision,
                         bool resulting_directory = false)
{
  return pkgapply::application_path_consequence(
      decision.path(),
      application_role(decision.role()),
      decision.active(),
      decision.rejected(),
      decision.incoming_entry(),
      decision.ownership(),
      pkgapply::application_effect_status::completed,
      pkgapply::application_effect_status::not_attempted,
      pkgapply::application_path_observation::absent(decision.path()),
      pkgapply::application_path_observation::present(
          resulting_directory ? completed_directory(decision.path())
                              : completed_regular(decision.path(), 1)),
      std::nullopt,
      pkgapply::ownership_publication_status::eligible);
}

pkgapply::application_path_consequence
upgrade_consequence(const pkgplan::upgrade_path_decision& decision)
{
  return pkgapply::application_path_consequence(
      decision.path(),
      application_role(decision.role()),
      decision.active(),
      decision.rejected(),
      decision.incoming_entry(),
      decision.ownership(),
      pkgapply::application_effect_status::completed,
      pkgapply::application_effect_status::not_attempted,
      pkgapply::application_path_observation::present(
          completed_regular(decision.path(), 2)),
      pkgapply::application_path_observation::present(
          completed_regular(decision.path(), 3)),
      std::nullopt,
      pkgapply::ownership_publication_status::eligible);
}

pkgapply::application_path_consequence
removal_consequence(const pkgplan::removal_path_decision& decision)
{
  return pkgapply::application_path_consequence(
      decision.path(),
      pkgapply::application_path_role::installed_owned_path,
      decision.active(),
      decision.rejected(),
      std::nullopt,
      decision.ownership(),
      pkgapply::application_effect_status::completed,
      pkgapply::application_effect_status::not_attempted,
      pkgapply::application_path_observation::present(
          completed_regular(decision.path(), 2)),
      pkgapply::application_path_observation::absent(decision.path()),
      std::nullopt,
      pkgapply::ownership_publication_status::eligible);
}

struct installation_fixture final {
  pkgstate::snapshot expected;
  planner_context planner;
  pkgplan::installation_plan plan;
  pkgapply::application_target_context target;
  pkgapply::installation_application_request request;
  pkgapply::lease_bound_state_projection projection;
  pkgapply::completed_application_evidence evidence;

  explicit installation_fixture(std::uint8_t target_seed = 1,
                                bool wrong_owners = false)
      : expected(pkgstate::snapshot::make(state_target(target_seed))),
        planner(),
        plan(installation_plan(expected, planner)),
        target(application_target(
            expected.target_binding(), planner, target_seed)),
        request(pkgapply::installation_application_request::make(
            plan, target, execution_control())),
        projection(application_projection(
            expected, plan, 30, wrong_owners)),
        evidence(pkgapply::completed_application_evidence::installation(
            request,
            apply_identity<pkgapply::application_attempt_identity>(40),
            projection.identity(),
            apply_identity<pkgapply::application_journal_identity>(41),
            {installation_consequence(plan.paths().front())},
            durability()))
  {
  }
};

struct upgrade_fixture final {
  pkgstate::state_target_binding target_binding;
  pkgstate::installed_package old_package;
  pkgstate::snapshot expected;
  planner_context planner;
  pkgplan::upgrade_plan plan;
  pkgapply::application_target_context target;
  pkgapply::upgrade_application_request request;
  pkgapply::lease_bound_state_projection projection;
  pkgapply::completed_application_evidence evidence;

  explicit upgrade_fixture(
      std::optional<pkgplan::installed_control_identity> control_override =
          std::nullopt)
      : target_binding(state_target()),
        old_package(state_package(
            target_binding,
            "1.0",
            {{pkgstate::package_path::parse("tool"),
              pkgstate::owned_entry_type::non_directory}})),
        expected(pkgstate::snapshot::make(target_binding, {old_package})),
        planner(),
        plan(upgrade_plan(
            expected, old_package, planner, control_override)),
        target(application_target(target_binding, planner)),
        request(pkgapply::upgrade_application_request::make(
            plan, target, execution_control())),
        projection(application_projection(expected, plan)),
        evidence(pkgapply::completed_application_evidence::upgrade(
            request,
            apply_identity<pkgapply::application_attempt_identity>(42),
            projection.identity(),
            apply_identity<pkgapply::application_journal_identity>(43),
            {upgrade_consequence(plan.paths().front())},
            durability()))
  {
  }
};

struct removal_fixture final {
  pkgstate::state_target_binding target_binding;
  pkgstate::installed_package old_package;
  pkgstate::snapshot expected;
  planner_context planner;
  pkgplan::removal_plan plan;
  pkgapply::application_target_context target;
  pkgapply::removal_application_request request;
  pkgapply::lease_bound_state_projection projection;
  pkgapply::completed_application_evidence evidence;

  removal_fixture()
      : target_binding(state_target()),
        old_package(state_package(
            target_binding,
            "1.0",
            {{pkgstate::package_path::parse("tool"),
              pkgstate::owned_entry_type::non_directory}})),
        expected(pkgstate::snapshot::make(target_binding, {old_package})),
        planner(),
        plan(removal_plan(expected, old_package, planner)),
        target(application_target(target_binding, planner)),
        request(pkgapply::removal_application_request::make(
            plan, target, execution_control())),
        projection(application_projection(expected, plan)),
        evidence(pkgapply::completed_application_evidence::removal(
            request,
            apply_identity<pkgapply::application_attempt_identity>(44),
            projection.identity(),
            apply_identity<pkgapply::application_journal_identity>(45),
            {removal_consequence(plan.paths().front())},
            durability()))
  {
  }
};

void
check_installation()
{
  installation_fixture fixture;
  const pkgstate::state_publication_request publication =
      pkgstate::apply_adapter::project_completed_application(
          fixture.expected,
          fixture.projection,
          pkgapply::package_application_request(fixture.request),
          fixture.evidence);

  CHECK(publication.expected_snapshot() == fixture.expected.identity());
  CHECK(publication.target_binding() == fixture.expected.target_binding());
  CHECK(publication.deltas().size() == 1);
  CHECK(!publication.transaction_evidence().has_value());

  const pkgstate::package_state_delta& delta = publication.deltas().front();
  CHECK(delta.kind() == pkgstate::package_state_delta_kind::install);
  CHECK(delta.package_name() == "tool");
  CHECK(!delta.expected_package().has_value());
  CHECK(delta.proposed_package().has_value());
  CHECK(delta.operation_plan().string() == fixture.plan.identity().string());
  CHECK(delta.application_evidence().string() ==
        fixture.evidence.identity().string());

  const pkgstate::installed_package& installed = *delta.proposed_package();
  CHECK(installed.release().name() == "tool");
  CHECK(installed.release().version() == "1.0");
  CHECK(installed.target_binding() == fixture.expected.target_binding());
  CHECK(installed.manifest().size() == 1);
  CHECK(installed.manifest().front().path.string() == "tool");
  CHECK(installed.manifest().front().type ==
        pkgstate::owned_entry_type::non_directory);

  const pkgstate::installed_control& control = installed.control();
  CHECK(control.runtime_dependencies().size() == 1);
  CHECK(control.runtime_dependencies().front().expression() == "libc >= 1");
  CHECK(control.removal_lifecycle().size() == 1);
  CHECK(control.removal_lifecycle().front().material() ==
        "prepare-new-remove");
  CHECK(control.target_profile().size() == 1);
  CHECK(control.provenance().size() == 4);
  CHECK(control.provenance()[0].kind() ==
        pkgstate::control_provenance_kind::candidate_control);
  CHECK(control.provenance()[1].kind() ==
        pkgstate::control_provenance_kind::artifact);
  CHECK(control.provenance()[2].kind() ==
        pkgstate::control_provenance_kind::artifact_manifest);
  CHECK(control.provenance()[3].kind() ==
        pkgstate::control_provenance_kind::application_evidence);
  CHECK(control.provenance()[0].identity() ==
        fixture.plan.publication().candidate().string());
  CHECK(control.provenance()[1].identity() ==
        fixture.plan.publication().artifact().string());
  CHECK(control.provenance()[2].identity() ==
        fixture.plan.publication().artifact_manifest().string());
  CHECK(control.provenance()[3].identity() ==
        fixture.evidence.identity().string());
  CHECK(control.completeness().runtime_dependencies ==
        pkgstate::installed_control_fact_state::recorded_at_installation);
  CHECK(control.completeness().removal_lifecycle ==
        pkgstate::installed_control_fact_state::recorded_at_installation);
  CHECK(control.completeness().target_profile ==
        pkgstate::installed_control_fact_state::recorded_at_installation);
  CHECK(control.completeness().provenance ==
        pkgstate::installed_control_fact_state::recorded_at_installation);
  CHECK(installed.release().identity().string() !=
        fixture.plan.publication().release().identity().string());

  const pkgstate::state_publication_request repeated =
      pkgstate::apply_adapter::project_completed_application(
          fixture.expected,
          fixture.projection,
          pkgapply::package_application_request(fixture.request),
          fixture.evidence);
  CHECK(repeated.identity() == publication.identity());
}

void
check_directory_classification()
{
  installation_fixture fixture;
  const pkgapply::completed_application_evidence evidence =
      pkgapply::completed_application_evidence::installation(
          fixture.request,
          apply_identity<pkgapply::application_attempt_identity>(111),
          fixture.projection.identity(),
          apply_identity<pkgapply::application_journal_identity>(112),
          {installation_consequence(fixture.plan.paths().front(), true)},
          durability());
  const pkgstate::state_publication_request publication =
      pkgstate::apply_adapter::project_completed_application(
          fixture.expected,
          fixture.projection,
          pkgapply::package_application_request(fixture.request),
          evidence);
  CHECK(publication.deltas().front().proposed_package().has_value());
  CHECK(publication.deltas().front().proposed_package()->manifest().front().type ==
        pkgstate::owned_entry_type::directory);
}

void
check_upgrade()
{
  upgrade_fixture fixture;
  const pkgstate::state_publication_request publication =
      pkgstate::apply_adapter::project_completed_application(
          fixture.expected,
          fixture.projection,
          pkgapply::package_application_request(fixture.request),
          fixture.evidence);
  CHECK(publication.deltas().size() == 1);
  const pkgstate::package_state_delta& delta = publication.deltas().front();
  CHECK(delta.kind() == pkgstate::package_state_delta_kind::replace);
  CHECK(delta.expected_package().has_value());
  CHECK(*delta.expected_package() == fixture.old_package.identity());
  CHECK(delta.proposed_package().has_value());
  CHECK(delta.proposed_package()->release().version() == "2.0");
  CHECK(delta.proposed_package()->identity() != fixture.old_package.identity());
}

void
check_removal()
{
  removal_fixture fixture;
  const pkgstate::state_publication_request publication =
      pkgstate::apply_adapter::project_completed_application(
          fixture.expected,
          fixture.projection,
          pkgapply::package_application_request(fixture.request),
          fixture.evidence);
  CHECK(publication.deltas().size() == 1);
  const pkgstate::package_state_delta& delta = publication.deltas().front();
  CHECK(delta.kind() == pkgstate::package_state_delta_kind::remove);
  CHECK(delta.expected_package().has_value());
  CHECK(*delta.expected_package() == fixture.old_package.identity());
  CHECK(!delta.proposed_package().has_value());
}

void
check_failures()
{
  installation_fixture fixture;

  const auto different_control = pkgapply::installation_application_request::make(
      fixture.plan,
      fixture.target,
      execution_control(
          pkgapply::application_durability_requirement::visibility_only));
  try
  {
    static_cast<void>(
        pkgstate::apply_adapter::project_completed_application(
            fixture.expected,
            fixture.projection,
            pkgapply::package_application_request(different_control),
            fixture.evidence));
    CHECK(false);
  }
  catch (const pkgstate::apply_adapter::projection_error& error)
  {
    CHECK(error.code() ==
          pkgstate::apply_adapter::projection_error_code::
              request_binding_mismatch);
  }

  const auto other_projection = application_projection(
      fixture.expected, fixture.plan, 80);
  try
  {
    static_cast<void>(
        pkgstate::apply_adapter::project_completed_application(
            fixture.expected,
            other_projection,
            pkgapply::package_application_request(fixture.request),
            fixture.evidence));
    CHECK(false);
  }
  catch (const pkgstate::apply_adapter::projection_error& error)
  {
    CHECK(error.code() ==
          pkgstate::apply_adapter::projection_error_code::
              state_projection_mismatch);
  }

  installation_fixture wrong_owners(1, true);
  try
  {
    static_cast<void>(
        pkgstate::apply_adapter::project_completed_application(
            wrong_owners.expected,
            wrong_owners.projection,
            pkgapply::package_application_request(wrong_owners.request),
            wrong_owners.evidence));
    CHECK(false);
  }
  catch (const pkgstate::apply_adapter::projection_error& error)
  {
    CHECK(error.code() ==
          pkgstate::apply_adapter::projection_error_code::
              ownership_projection_mismatch);
  }

  const pkgstate::snapshot other_state =
      pkgstate::snapshot::make(state_target(20));
  try
  {
    static_cast<void>(
        pkgstate::apply_adapter::project_completed_application(
            other_state,
            fixture.projection,
            pkgapply::package_application_request(fixture.request),
            fixture.evidence));
    CHECK(false);
  }
  catch (const pkgstate::apply_adapter::projection_error& error)
  {
    CHECK(error.code() ==
          pkgstate::apply_adapter::projection_error_code::
              expected_state_mismatch);
  }

  const auto incomplete_projection = application_projection(
      fixture.expected,
      fixture.plan,
      90,
      false,
      pkgapply::state_projection_completeness::incomplete);
  const auto incomplete_evidence =
      pkgapply::completed_application_evidence::installation(
          fixture.request,
          apply_identity<pkgapply::application_attempt_identity>(91),
          incomplete_projection.identity(),
          apply_identity<pkgapply::application_journal_identity>(92),
          {installation_consequence(fixture.plan.paths().front())},
          durability());
  try
  {
    static_cast<void>(
        pkgstate::apply_adapter::project_completed_application(
            fixture.expected,
            incomplete_projection,
            pkgapply::package_application_request(fixture.request),
            incomplete_evidence));
    CHECK(false);
  }
  catch (const pkgstate::apply_adapter::projection_error& error)
  {
    CHECK(error.code() ==
          pkgstate::apply_adapter::projection_error_code::
              expected_state_mismatch);
  }

  const auto foreign_target = pkgapply::application_target_context::make(
      fixture.planner.target,
      apply_identity<pkgapply::managed_target_identity>(100),
      translate_identity<pkgapply::root_view_identity>(
          fixture.expected.target_binding().root_view()),
      apply_identity<pkgapply::observation_backend_identity>(101),
      apply_identity<pkgapply::mutation_backend_identity>(102),
      apply_identity<pkgapply::mutation_exclusion_domain_identity>(103),
      apply_identity<pkgapply::active_object_namespace_identity>(104),
      apply_identity<pkgapply::rejected_object_store_identity>(105),
      apply_identity<pkgapply::staging_namespace_identity>(106),
      apply_identity<pkgapply::journal_namespace_identity>(107),
      apply_identity<pkgapply::execution_capability_profile_identity>(108));
  const auto foreign_request = pkgapply::installation_application_request::make(
      fixture.plan, foreign_target, execution_control());
  const auto foreign_evidence =
      pkgapply::completed_application_evidence::installation(
          foreign_request,
          apply_identity<pkgapply::application_attempt_identity>(109),
          fixture.projection.identity(),
          apply_identity<pkgapply::application_journal_identity>(110),
          {installation_consequence(fixture.plan.paths().front())},
          durability());
  try
  {
    static_cast<void>(
        pkgstate::apply_adapter::project_completed_application(
            fixture.expected,
            fixture.projection,
            pkgapply::package_application_request(foreign_request),
            foreign_evidence));
    CHECK(false);
  }
  catch (const pkgstate::apply_adapter::projection_error& error)
  {
    CHECK(error.code() ==
          pkgstate::apply_adapter::projection_error_code::
              target_binding_mismatch);
  }

  upgrade_fixture wrong_control(
      plan_identity<pkgplan::installed_control_identity>(120));
  try
  {
    static_cast<void>(
        pkgstate::apply_adapter::project_completed_application(
            wrong_control.expected,
            wrong_control.projection,
            pkgapply::package_application_request(wrong_control.request),
            wrong_control.evidence));
    CHECK(false);
  }
  catch (const pkgstate::apply_adapter::projection_error& error)
  {
    CHECK(error.code() ==
          pkgstate::apply_adapter::projection_error_code::
              package_state_mismatch);
  }
}

} // namespace

int
main()
{
  check_installation();
  check_directory_classification();
  check_upgrade();
  check_removal();
  check_failures();
  return 0;
}
