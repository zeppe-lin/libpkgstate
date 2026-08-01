// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "test.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <libpkgapply/incoming_package.h>
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
#include <libpkgstate-apply/state_projection.h>
#include <libpkgstate/canonical_store.h>
#include <libpkgstate-build/adapter.h>
#include <libpkgstate-source/adapter.h>
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

pkgsource::source_snapshot
source_snapshot(const char* version,
                const char* removal_program = "prepare-new-remove")
{
  using namespace pkgsource;
  return seal_source(
      source_origin("recipe.yml"), source_syntax::recipe_yaml_v1,
      recipe_declaration(
          package_release(package_reference("tool"), version, 1),
          package_metadata("Tool", std::nullopt, std::nullopt,
                           {"GPL-3.0-or-later"}),
          {},
          program(program_language::posix_shell,
                  "install -m755 tool \"$PKG/tool\"\n"),
          {requirement_declaration(
              requirement_scope::run(),
              requirement_subject(package_reference("libc")),
              declaration_provenance(
                  "recipe.yml", "requirements.run[0]", 10, 3))},
          {lifecycle_program(
              lifecycle_action::pre_remove,
              program(program_language::posix_shell, removal_program))},
          architecture_requirements(
              {architecture_reference("x86_64")},
              {architecture_reference("x86_64")}),
          declaration_provenance("recipe.yml", "$", 1, 1)),
      profile_catalog::seal({}));
}

pkgstate::package_source_record
state_source(const char* version, std::uint8_t,
             const char* removal_program = "prepare-new-remove")
{
  const pkgsource::source_snapshot source =
      source_snapshot(version, removal_program);
  return pkgstate::source_adapter::project_source(
      source, pkgsource::architecture_reference("x86_64"),
      pkgsource::architecture_reference("x86_64"));
}

pkgstate::installed_package
state_package(const pkgstate::state_target_binding& target,
              const char* version,
              std::vector<pkgstate::owned_entry> manifest)
{
  const std::uint8_t release_seed = 69;
  pkgstate::package_source_record source =
      state_source(version, release_seed, "finish-old-remove");
  pkgstate::installed_control control = pkgstate::installed_control::make(
      source,
      pkgstate::installation_reason::runtime_dependency(
          pkgstate::package_reference("base-system")),
      pkgstate::build_provenance(
          source.identity(),
          state_identity<pkgstate::build_request_identity>(90),
          state_identity<pkgstate::source_material_set_identity>(91),
          state_identity<pkgstate::build_input_set_identity>(92),
          state_identity<pkgstate::environment_policy_identity>(93),
          state_identity<pkgstate::build_policy_identity>(94),
          state_identity<pkgstate::build_result_identity>(95),
          state_identity<pkgstate::payload_manifest_identity>(96),
          state_identity<pkgstate::build_artifact_identity>(97),
          state_identity<pkgstate::artifact_content_identity>(98),
          state_identity<pkgstate::artifact_binding_identity>(99),
          state_identity<pkgstate::execution_evidence_identity>(100),
          state_identity<pkgstate::artifact_image_identity>(101),
          state_identity<pkgstate::artifact_inspection_identity>(102)));
  return pkgstate::installed_package::make(
      pkgstate::installation_receipt::make(
          std::move(control), target, std::move(manifest),
          state_identity<pkgstate::operation_plan_identity>(96),
          state_identity<pkgstate::application_evidence_identity>(97)));
}

pkgstate::installed_object_metadata
state_regular(std::uint8_t content)
{
  return pkgstate::installed_object_metadata(
      pkgstate::owned_object_kind::regular, 0755, 0, 0,
      pkgstate::installed_object_timestamp(10, 0), std::uint64_t{4},
      state_identity<pkgstate::installed_regular_content_identity>(content));
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

pkgplan::installed_control_projection
planner_control(const pkgstate::installed_control& source)
{
  std::vector<pkgplan::runtime_dependency_declaration> dependencies;
  for (const auto& item : source.source().runtime_requirements())
    dependencies.push_back(
        pkgplan::runtime_dependency_declaration::make(
            item.package().name()));

  std::vector<pkgplan::removal_lifecycle_declaration> lifecycle;
  for (const pkgstate::lifecycle_action action : {
           pkgstate::lifecycle_action::pre_remove,
           pkgstate::lifecycle_action::post_remove})
  {
    const pkgstate::lifecycle_program* item = source.source().lifecycle(action);
    if (item == nullptr)
      continue;
    lifecycle.push_back(pkgplan::removal_lifecycle_declaration::make(
        action == pkgstate::lifecycle_action::pre_remove
            ? pkgplan::removal_lifecycle_phase::pre_remove
            : pkgplan::removal_lifecycle_phase::post_remove,
        "text/x-posix-shell", item->value().material()));
  }

  pkgplan::installed_control_completeness completeness;
  completeness.runtime_dependencies =
      pkgplan::control_fact_availability::known;
  completeness.removal_lifecycle =
      pkgplan::control_fact_availability::known;
  completeness.target_profile =
      pkgplan::control_fact_availability::known;
  return pkgplan::installed_control_projection(
      completeness, std::move(dependencies), std::move(lifecycle),
      {pkgplan::target_profile_fact::make(
          "pkgsource.target-architectures", "x86_64")});
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
  entry.mtime = 10;
  entry.mtime_nanoseconds = 0;
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

std::string
byte_digest(std::uint8_t byte)
{
  constexpr char hex[] = "0123456789abcdef";
  std::string result;
  result.reserve(64);
  for (std::size_t index = 0; index < 32; ++index)
  {
    result.push_back(hex[(byte >> 4) & 0x0f]);
    result.push_back(hex[byte & 0x0f]);
  }
  return result;
}

pkgapply::incoming_package_authority
incoming_authority(const char* version, std::uint8_t content)
{
  const pkgsource::source_snapshot source = source_snapshot(version);
  const pkgbuild::build_request request = pkgbuild::build_request::seal(
      source, {}, {}, pkgsource::architecture_reference("x86_64"),
      pkgsource::architecture_reference("x86_64"),
      pkgbuild::build_policy::make(
          pkgbuild::environment_policy::hermetic(1, 0022, 1700000000)));
  const pkgbuild::payload_manifest payload = pkgbuild::payload_manifest::seal({
      pkgbuild::payload_entry::regular(
          pkgbuild::payload_path::parse("tool"), 0755, 0, 0, 4,
          pkgbuild::payload_time{10, 0},
          pkgbuild::sha256_digest(byte_digest(content))),
  });
  pkgimage::inspected_package_image image = incoming_image(content);
  const pkgbuild::sealed_artifact artifact = pkgbuild::sealed_artifact::make(
      pkgbuild::artifact_encoding::package_tar_v1,
      pkgbuild::artifact_compression::none, 4,
      pkgbuild::sha256_digest(
          byte_digest(static_cast<std::uint8_t>(content + 30))));
  pkgbuild::build_result result = pkgbuild::build_result::succeeded(
      request, payload, artifact,
      pkgbuild::execution_evidence_identity::from_sha256(
          byte_digest(static_cast<std::uint8_t>(content + 60))));
  return pkgapply::incoming_package_authority::admit(
      std::move(result), std::move(image));
}

pkgstate::build_adapter::build_authority
state_build_authority(const pkgapply::incoming_package_authority& incoming)
{
  const pkgbuild::build_request& request = incoming.build().request();
  return pkgstate::build_adapter::project_build(
      pkgstate::source_adapter::project_source(
          request.source(), request.architectures().build(),
          request.architectures().target()),
      incoming.build(), incoming.image());
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
                  const planner_context& context,
                  const pkgapply::incoming_package_authority& incoming)
{
  const pkgplan::package_path path = pkgplan::package_path::parse("tool");
  const pkgplan::package_release& release = incoming.candidate().release();
  const auto archive = incoming.image().receipt().archive_digest();

  pkgplan::installation_request request(
      incoming.candidate(),
      pkgplan::artifact_package_fact(
          translate_identity<pkgplan::artifact_identity>(archive),
          plan_identity<pkgplan::artifact_manifest_identity>(73),
          release),
      archive,
      incoming.image(),
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
          release.name(), release.version(), std::to_string(release.release())),
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
             const pkgapply::incoming_package_authority& incoming,
             std::optional<pkgplan::installed_control_identity>
                 control_override = std::nullopt)
{
  const pkgplan::package_path path = pkgplan::package_path::parse("tool");
  const pkgplan::package_release& release = incoming.candidate().release();
  const auto archive = incoming.image().receipt().archive_digest();

  pkgplan::upgrade_request request(
      planner_installed(expected, installed, control_override),
      incoming.candidate(),
      pkgplan::artifact_package_fact(
          translate_identity<pkgplan::artifact_identity>(archive),
          plan_identity<pkgplan::artifact_manifest_identity>(77),
          release),
      archive,
      incoming.image(),
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



class recording_lease final : public pkgapply::target_mutation_lease {
public:
  recording_lease(
      pkgapply::mutation_lease_instance_identity identity,
      pkgapply::application_target_context_identity target,
      pkgapply::mutation_exclusion_domain_identity domain,
      std::vector<bool> held_results,
      std::vector<std::string>& trace)
      : identity_(std::move(identity)), target_(std::move(target)),
        domain_(std::move(domain)), held_results_(std::move(held_results)),
        trace_(trace)
  {
  }

  const pkgapply::mutation_lease_instance_identity&
  identity() const noexcept override
  {
    return identity_;
  }

  const pkgapply::application_target_context_identity&
  target() const noexcept override
  {
    return target_;
  }

  const pkgapply::mutation_exclusion_domain_identity&
  exclusion_domain() const noexcept override
  {
    return domain_;
  }

  bool held() const noexcept override
  {
    trace_.push_back("held");
    const std::size_t index = held_calls_++;
    if (held_results_.empty())
      return false;
    return held_results_[std::min(index, held_results_.size() - 1)];
  }

private:
  pkgapply::mutation_lease_instance_identity identity_;
  pkgapply::application_target_context_identity target_;
  pkgapply::mutation_exclusion_domain_identity domain_;
  std::vector<bool> held_results_;
  std::vector<std::string>& trace_;
  mutable std::size_t held_calls_ = 0;
};

class recording_store final : public pkgstate::canonical_store {
public:
  recording_store(pkgstate::snapshot state, std::vector<std::string>& trace)
      : state_(std::move(state)), trace_(trace)
  {
  }

  pkgstate::snapshot read() const override
  {
    trace_.push_back("read");
    ++reads_;
    return state_;
  }

  std::size_t reads() const noexcept { return reads_; }

protected:
  std::unique_ptr<pkgstate::canonical_publication_transaction>
  begin_publication() const override
  {
    throw std::runtime_error("unexpected publication");
  }

private:
  pkgstate::snapshot state_;
  std::vector<std::string>& trace_;
  mutable std::size_t reads_ = 0;
};

struct installation_fixture final {
  pkgstate::snapshot expected;
  planner_context planner;
  pkgapply::incoming_package_authority incoming;
  pkgplan::installation_plan plan;
  pkgapply::application_target_context target;
  pkgapply::installation_application_request request;
  pkgapply::lease_bound_state_projection projection;
  pkgapply::completed_application_evidence evidence;

  explicit installation_fixture(std::uint8_t target_seed = 1,
                                bool wrong_owners = false)
      : expected(pkgstate::snapshot::make(state_target(target_seed))),
        planner(),
        incoming(incoming_authority("1.0", 1)),
        plan(installation_plan(expected, planner, incoming)),
        target(application_target(
            expected.target_binding(), planner, target_seed)),
        request(pkgapply::installation_application_request::make(
            plan, incoming, target, execution_control())),
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
  pkgapply::incoming_package_authority incoming;
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
            {pkgstate::owned_entry::make(
              pkgstate::package_path::parse("tool"), state_regular(2),
              pkgstate::active_object_origin::incoming_payload)})),
        expected(pkgstate::snapshot::make(target_binding, {old_package})),
        planner(),
        incoming(incoming_authority("2.0", 3)),
        plan(upgrade_plan(
            expected, old_package, planner, incoming, control_override)),
        target(application_target(target_binding, planner)),
        request(pkgapply::upgrade_application_request::make(
            plan, incoming, target, execution_control())),
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
            {pkgstate::owned_entry::make(
              pkgstate::package_path::parse("tool"), state_regular(2),
              pkgstate::active_object_origin::incoming_payload)})),
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
          fixture.request,
          fixture.evidence,
          pkgstate::installation_reason::explicit_request());

  CHECK(publication.expected_snapshot() == fixture.expected.identity());
  CHECK(publication.target_binding() == fixture.expected.target_binding());
  CHECK(publication.deltas().size() == 1);
  const pkgstate::package_state_delta& delta = publication.deltas().front();
  CHECK(delta.kind() == pkgstate::package_state_delta_kind::install);
  CHECK(delta.package_name() == "tool");
  CHECK(!delta.expected_package().has_value());
  CHECK(delta.proposed_package().has_value());

  const pkgstate::installed_package& installed = *delta.proposed_package();
  CHECK(installed.release().name() == "tool");
  CHECK(installed.release().version() == "1.0");
  CHECK(installed.release().identity().string() ==
        fixture.plan.publication().release().identity().string());
  CHECK(installed.target_binding() == fixture.expected.target_binding());
  CHECK(installed.manifest().size() == 1);
  CHECK(installed.manifest().front().path().string() == "tool");
  CHECK(installed.manifest().front().kind() ==
        pkgstate::owned_object_kind::regular);
  CHECK(installed.manifest().front().origin() ==
        pkgstate::active_object_origin::incoming_payload);
  const pkgstate::installed_object_metadata& object =
      installed.manifest().front().object();
  CHECK(object.mode() == 0755);
  CHECK(object.uid() == 0);
  CHECK(object.gid() == 0);
  CHECK(object.size().has_value() && *object.size() == 4);
  CHECK(object.mtime().seconds() == 10);
  CHECK(object.mtime().nanoseconds() == 0);
  CHECK(object.regular_content().has_value());

  const pkgstate::installed_control& control = installed.control();
  CHECK(control.source().runtime_requirements().size() == 1);
  CHECK(control.source().runtime_requirements().front().package().name() ==
        "libc");
  CHECK(control.reason().kind() ==
        pkgstate::installation_reason_kind::explicit_request);
  const auto authority = state_build_authority(fixture.request.incoming());
  CHECK(control.build() == authority.provenance());
  CHECK(control.build().artifact_content().string() ==
        fixture.plan.publication().artifact().string());
  CHECK(installed.receipt().operation_plan().string() ==
        fixture.plan.identity().string());
  CHECK(installed.receipt().application_evidence().string() ==
        fixture.evidence.identity().string());

  CHECK(!publication.transaction_evidence().has_value());
  CHECK(!installed.receipt().transaction_evidence().has_value());

  const auto transaction =
      state_identity<pkgstate::transaction_evidence_identity>(200);
  const pkgstate::state_publication_request composed =
      pkgstate::apply_adapter::project_completed_application(
          fixture.expected,
          fixture.projection,
          fixture.request,
          fixture.evidence,
          pkgstate::installation_reason::explicit_request(),
          transaction);
  CHECK(composed.transaction_evidence().has_value());
  CHECK(*composed.transaction_evidence() == transaction);
  CHECK(composed.deltas().front().proposed_package().has_value());
  CHECK(composed.deltas().front().proposed_package()->receipt()
            .transaction_evidence().has_value());
  CHECK(*composed.deltas().front().proposed_package()->receipt()
             .transaction_evidence() == transaction);
  CHECK(composed.identity() != publication.identity());

  const pkgstate::state_publication_request repeated =
      pkgstate::apply_adapter::project_completed_application(
          fixture.expected,
          fixture.projection,
          fixture.request,
          fixture.evidence,
          pkgstate::installation_reason::explicit_request());
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
          fixture.request,
          evidence, pkgstate::installation_reason::explicit_request());
  CHECK(publication.deltas().front().proposed_package().has_value());
  CHECK(publication.deltas().front().proposed_package()->manifest().front().kind() ==
        pkgstate::owned_object_kind::directory);
}

void
check_upgrade()
{
  upgrade_fixture fixture;
  const pkgstate::state_publication_request publication =
      pkgstate::apply_adapter::project_completed_application(
          fixture.expected,
          fixture.projection,
          fixture.request,
          fixture.evidence);
  CHECK(publication.deltas().size() == 1);
  const pkgstate::package_state_delta& delta = publication.deltas().front();
  CHECK(delta.kind() == pkgstate::package_state_delta_kind::replace);
  CHECK(delta.expected_package().has_value());
  CHECK(*delta.expected_package() == fixture.old_package.identity());
  CHECK(delta.proposed_package().has_value());
  CHECK(delta.proposed_package()->release().version() == "2.0");
  CHECK(delta.proposed_package()->identity() != fixture.old_package.identity());
  CHECK(delta.proposed_package()->control().reason() ==
        fixture.old_package.control().reason());
  CHECK(!publication.transaction_evidence().has_value());
  CHECK(!delta.proposed_package()->receipt().transaction_evidence().has_value());

  const auto transaction =
      state_identity<pkgstate::transaction_evidence_identity>(201);
  const pkgstate::state_publication_request composed =
      pkgstate::apply_adapter::project_completed_application(
          fixture.expected,
          fixture.projection,
          fixture.request,
          fixture.evidence,
          transaction);
  CHECK(composed.transaction_evidence().has_value());
  CHECK(*composed.transaction_evidence() == transaction);
  CHECK(composed.deltas().front().proposed_package().has_value());
  CHECK(composed.deltas().front().proposed_package()->receipt()
            .transaction_evidence().has_value());
  CHECK(*composed.deltas().front().proposed_package()->receipt()
             .transaction_evidence() == transaction);
}

void
check_removal()
{
  removal_fixture fixture;
  const pkgstate::state_publication_request publication =
      pkgstate::apply_adapter::project_completed_application(
          fixture.expected,
          fixture.projection,
          fixture.request,
          fixture.evidence);
  CHECK(publication.deltas().size() == 1);
  const pkgstate::package_state_delta& delta = publication.deltas().front();
  CHECK(delta.kind() == pkgstate::package_state_delta_kind::remove);
  CHECK(delta.expected_package().has_value());
  CHECK(*delta.expected_package() == fixture.old_package.identity());
  CHECK(!delta.proposed_package().has_value());
  CHECK(!publication.transaction_evidence().has_value());

  const auto transaction =
      state_identity<pkgstate::transaction_evidence_identity>(202);
  const pkgstate::state_publication_request composed =
      pkgstate::apply_adapter::project_completed_application(
          fixture.expected,
          fixture.projection,
          fixture.request,
          fixture.evidence,
          transaction);
  CHECK(composed.transaction_evidence().has_value());
  CHECK(*composed.transaction_evidence() == transaction);
  CHECK(composed.deltas().front().kind() ==
        pkgstate::package_state_delta_kind::remove);
}

void
check_failures()
{
  installation_fixture fixture;

  try
  {
    static_cast<void>(pkgapply::installation_application_request::make(
        fixture.plan, incoming_authority("1.0", 2), fixture.target,
        execution_control()));
    CHECK(false);
  }
  catch (const pkgapply::incoming_package_error& error)
  {
    CHECK(error.code() ==
          pkgapply::incoming_package_error_code::plan_binding);
  }

  const auto different_control = pkgapply::installation_application_request::make(
      fixture.plan,
      fixture.incoming,
      fixture.target,
      execution_control(
          pkgapply::application_durability_requirement::visibility_only));
  try
  {
    static_cast<void>(
        pkgstate::apply_adapter::project_completed_application(
            fixture.expected,
            fixture.projection,
            different_control,
            fixture.evidence,
            pkgstate::installation_reason::explicit_request()));
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
            fixture.request,
            fixture.evidence,
            pkgstate::installation_reason::explicit_request()));
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
            wrong_owners.request,
            wrong_owners.evidence,
            pkgstate::installation_reason::explicit_request()));
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
            fixture.request,
            fixture.evidence,
            pkgstate::installation_reason::explicit_request()));
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
            fixture.request,
            incomplete_evidence,
            pkgstate::installation_reason::explicit_request()));
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
      fixture.plan, fixture.incoming, foreign_target, execution_control());
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
            foreign_request,
            foreign_evidence,
            pkgstate::installation_reason::explicit_request()));
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
            wrong_control.request,
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



void
check_lease_bound_state_projection()
{
  removal_fixture fixture;
  const pkgapply::package_application_request request(fixture.request);
  std::vector<std::string> trace;
  recording_lease lease(
      apply_identity<pkgapply::mutation_lease_instance_identity>(130),
      fixture.target.identity(), fixture.target.mutation_exclusion_domain(),
      {true}, trace);
  recording_store store(fixture.expected, trace);

  const auto projected =
      pkgstate::apply_adapter::read_application_state(request, lease, store);

  CHECK(store.reads() == 1);
  CHECK(trace == std::vector<std::string>({"held", "read", "held", "held"}));
  CHECK(projected.state().identity() == fixture.expected.identity());
  CHECK(projected.projection().lease() == lease.identity());
  CHECK(projected.projection().snapshot().string() ==
        fixture.expected.identity().string());
  CHECK(projected.projection().ownership_inventory().string() ==
        fixture.expected.ownership_identity().string());
  CHECK(projected.projection().completeness() ==
        pkgapply::state_projection_completeness::complete);
  CHECK(projected.projection().paths().size() ==
        fixture.plan.preconditions().paths().size());
  CHECK(projected.projection().paths().front().path() ==
        fixture.plan.preconditions().paths().front().path());
  CHECK(projected.projection().paths().front().owners() ==
        fixture.plan.preconditions().paths().front().owners());

  std::vector<std::string> repeated_trace;
  recording_lease repeated_lease(
      lease.identity(), fixture.target.identity(),
      fixture.target.mutation_exclusion_domain(), {true}, repeated_trace);
  recording_store repeated_store(fixture.expected, repeated_trace);
  const auto repeated = pkgstate::apply_adapter::read_application_state(
      request, repeated_lease, repeated_store);
  CHECK(repeated.projection().identity() == projected.projection().identity());
  CHECK(repeated.projection().evidence() == projected.projection().evidence());

  std::vector<std::string> other_trace;
  recording_lease other_lease(
      apply_identity<pkgapply::mutation_lease_instance_identity>(131),
      fixture.target.identity(), fixture.target.mutation_exclusion_domain(),
      {true}, other_trace);
  recording_store other_store(fixture.expected, other_trace);
  const auto other = pkgstate::apply_adapter::read_application_state(
      request, other_lease, other_store);
  CHECK(other.projection().identity() != projected.projection().identity());
  CHECK(other.projection().evidence() != projected.projection().evidence());
}

void
check_lease_bound_state_projection_failures()
{
  removal_fixture fixture;
  const pkgapply::package_application_request request(fixture.request);

  {
    std::vector<std::string> trace;
    recording_lease lease(
        apply_identity<pkgapply::mutation_lease_instance_identity>(140),
        fixture.target.identity(), fixture.target.mutation_exclusion_domain(),
        {false}, trace);
    recording_store store(fixture.expected, trace);
    try
    {
      static_cast<void>(pkgstate::apply_adapter::read_application_state(
          request, lease, store));
      CHECK(false);
    }
    catch (const pkgstate::apply_adapter::application_state_projection_error& error)
    {
      CHECK(error.code() ==
            pkgstate::apply_adapter::application_state_projection_error_code::
                lease_not_held);
      CHECK(store.reads() == 0);
    }
  }

  {
    std::vector<std::string> trace;
    recording_lease lease(
        apply_identity<pkgapply::mutation_lease_instance_identity>(141),
        fixture.target.identity(), fixture.target.mutation_exclusion_domain(),
        {true, false}, trace);
    recording_store store(fixture.expected, trace);
    try
    {
      static_cast<void>(pkgstate::apply_adapter::read_application_state(
          request, lease, store));
      CHECK(false);
    }
    catch (const pkgstate::apply_adapter::application_state_projection_error& error)
    {
      CHECK(error.code() ==
            pkgstate::apply_adapter::application_state_projection_error_code::
                lease_lost);
      CHECK(store.reads() == 1);
    }
  }

  {
    std::vector<std::string> trace;
    recording_lease lease(
        apply_identity<pkgapply::mutation_lease_instance_identity>(142),
        apply_identity<pkgapply::application_target_context_identity>(143),
        fixture.target.mutation_exclusion_domain(), {true}, trace);
    recording_store store(fixture.expected, trace);
    try
    {
      static_cast<void>(pkgstate::apply_adapter::read_application_state(
          request, lease, store));
      CHECK(false);
    }
    catch (const pkgstate::apply_adapter::application_state_projection_error& error)
    {
      CHECK(error.code() ==
            pkgstate::apply_adapter::application_state_projection_error_code::
                lease_target_mismatch);
      CHECK(store.reads() == 0);
    }
  }

  {
    std::vector<std::string> trace;
    recording_lease lease(
        apply_identity<pkgapply::mutation_lease_instance_identity>(144),
        fixture.target.identity(),
        apply_identity<pkgapply::mutation_exclusion_domain_identity>(145),
        {true}, trace);
    recording_store store(fixture.expected, trace);
    try
    {
      static_cast<void>(pkgstate::apply_adapter::read_application_state(
          request, lease, store));
      CHECK(false);
    }
    catch (const pkgstate::apply_adapter::application_state_projection_error& error)
    {
      CHECK(error.code() ==
            pkgstate::apply_adapter::application_state_projection_error_code::
                lease_domain_mismatch);
      CHECK(store.reads() == 0);
    }
  }

  {
    std::vector<std::string> trace;
    recording_lease lease(
        apply_identity<pkgapply::mutation_lease_instance_identity>(146),
        fixture.target.identity(), fixture.target.mutation_exclusion_domain(),
        {true}, trace);
    recording_store store(pkgstate::snapshot::make(state_target(20)), trace);
    try
    {
      static_cast<void>(pkgstate::apply_adapter::read_application_state(
          request, lease, store));
      CHECK(false);
    }
    catch (const pkgstate::apply_adapter::application_state_projection_error& error)
    {
      CHECK(error.code() ==
            pkgstate::apply_adapter::application_state_projection_error_code::
                target_binding_mismatch);
      CHECK(store.reads() == 1);
    }
  }

  {
    std::vector<std::string> trace;
    recording_lease lease(
        apply_identity<pkgapply::mutation_lease_instance_identity>(147),
        fixture.target.identity(), fixture.target.mutation_exclusion_domain(),
        {true}, trace);
    recording_store store(
        pkgstate::snapshot::make(fixture.target_binding), trace);
    try
    {
      static_cast<void>(pkgstate::apply_adapter::read_application_state(
          request, lease, store));
      CHECK(false);
    }
    catch (const pkgstate::apply_adapter::application_state_projection_error& error)
    {
      CHECK(error.code() ==
            pkgstate::apply_adapter::application_state_projection_error_code::
                expected_snapshot_mismatch);
      CHECK(store.reads() == 1);
    }
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
  check_lease_bound_state_projection();
  check_lease_bound_state_projection_failures();
  return 0;
}
