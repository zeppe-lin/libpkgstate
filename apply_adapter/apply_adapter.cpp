// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate-apply/adapter.h>

#include <algorithm>
#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <libpkgplan/control.h>
#include <libpkgplan/install.h>
#include <libpkgplan/remove.h>
#include <libpkgplan/upgrade.h>
#include <libpkgstate-build/adapter.h>
#include <libpkgstate-source/adapter.h>
#include <libpkgstate/installation_receipt.h>
#include <libpkgstate/installed_package.h>
#include <libpkgstate/owned_entry.h>
#include <libpkgstate/package_path.h>

namespace pkgstate::apply_adapter {
namespace {

template<typename Destination, typename Source>
Destination translate_identity(const Source& source)
{
  try
  {
    return Destination::parse(source.string());
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::identity_translation,
        std::string("state identity vocabulary rejected external identity: ") +
            error.what());
  }
}

package_path translate_path(const pkgplan::package_path& source)
{
  try
  {
    return package_path::parse(source.string());
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::path_translation,
        std::string("state path vocabulary rejected application path: ") +
            error.what());
  }
}

template<typename Left, typename Right>
bool same_identity_text(const Left& lhs, const Right& rhs)
{
  return lhs.string() == rhs.string();
}

const pkgplan::operation_preconditions& preconditions(
    const pkgapply::package_application_request& request)
{
  if (const auto* install = request.installation())
    return install->plan().preconditions();
  if (const auto* upgrade = request.upgrade())
    return upgrade->plan().preconditions();
  if (const auto* removal = request.removal())
    return removal->plan().preconditions();
  throw projection_error(projection_error_code::operation_binding_mismatch,
                         "application request has no operation body");
}

void validate_common(
    const snapshot& expected_state,
    const pkgapply::lease_bound_state_projection& application_state,
    const pkgapply::package_application_request& request,
    const pkgapply::completed_application_evidence& evidence)
{
  if (evidence.request() != request.identity() ||
      evidence.control() != request.control().identity())
  {
    throw projection_error(
        projection_error_code::request_binding_mismatch,
        "completed evidence does not name the supplied application request");
  }
  if (evidence.kind() != request.kind())
  {
    throw projection_error(
        projection_error_code::operation_binding_mismatch,
        "completed evidence operation differs from the application request");
  }
  if (evidence.plan() != request.plan())
  {
    throw projection_error(
        projection_error_code::plan_binding_mismatch,
        "completed evidence does not name the accepted operation plan");
  }
  if (evidence.target() != request.target().identity())
  {
    throw projection_error(
        projection_error_code::target_binding_mismatch,
        "completed evidence does not name the application target context");
  }
  if (evidence.state_projection() != application_state.identity())
  {
    throw projection_error(
        projection_error_code::state_projection_mismatch,
        "completed evidence does not name the supplied lease projection");
  }
  if (application_state.completeness() !=
      pkgapply::state_projection_completeness::complete)
  {
    throw projection_error(projection_error_code::expected_state_mismatch,
                           "application state projection is incomplete");
  }

  const auto& required = preconditions(request);
  if (required.installed_snapshot() != application_state.snapshot() ||
      required.ownership_inventory() !=
          application_state.ownership_inventory() ||
      required.target() != request.target().target())
  {
    throw projection_error(
        projection_error_code::plan_binding_mismatch,
        "application projection does not satisfy accepted plan authorities");
  }

  if (!same_identity_text(application_state.snapshot(),
                          expected_state.identity()) ||
      !same_identity_text(application_state.ownership_inventory(),
                          expected_state.ownership_identity()))
  {
    throw projection_error(
        projection_error_code::expected_state_mismatch,
        "application projection does not name canonical expected state");
  }

  const state_target_binding& state_target = expected_state.target_binding();
  if (!same_identity_text(request.target().managed_target(),
                          state_target.managed_target()) ||
      !same_identity_text(request.target().root_view(),
                          state_target.root_view()))
  {
    throw projection_error(
        projection_error_code::target_binding_mismatch,
        "application target does not match canonical state target");
  }

  if (application_state.paths().size() != evidence.paths().size())
  {
    throw projection_error(
        projection_error_code::ownership_projection_mismatch,
        "lease projection and completed evidence path universes differ");
  }

  for (const pkgapply::application_path_consequence& consequence :
       evidence.paths())
  {
    const pkgapply::projected_path_owners* projected =
        application_state.find(consequence.path());
    if (projected == nullptr)
    {
      throw projection_error(
          projection_error_code::ownership_projection_mismatch,
          "completed path is absent from lease-bound state projection");
    }

    const package_path state_path = translate_path(consequence.path());
    std::vector<pkgplan::installed_package_identity> expected_owners;
    for (const installed_package* owner : expected_state.owners(state_path))
    {
      expected_owners.push_back(
          translate_identity<pkgplan::installed_package_identity>(
              owner->identity()));
    }
    std::sort(expected_owners.begin(), expected_owners.end());
    if (projected->owners() != expected_owners)
    {
      throw projection_error(
          projection_error_code::ownership_projection_mismatch,
          "lease-projected path owners differ from canonical state");
    }
  }
}

std::string architecture_value(
    const std::vector<architecture_reference>& architectures)
{
  if (architectures.empty())
    return "*";
  std::string result;
  for (const architecture_reference& architecture : architectures)
  {
    if (!result.empty())
      result.push_back(',');
    result += architecture.name();
  }
  return result;
}

pkgplan::removal_lifecycle_phase planner_phase(lifecycle_action action)
{
  switch (action)
  {
    case lifecycle_action::pre_remove:
      return pkgplan::removal_lifecycle_phase::pre_remove;
    case lifecycle_action::post_remove:
      return pkgplan::removal_lifecycle_phase::post_remove;
    case lifecycle_action::pre_install:
    case lifecycle_action::post_install:
      break;
  }
  throw projection_error(projection_error_code::incoming_authority_mismatch,
                         "installation lifecycle is not planner removal control");
}

pkgplan::candidate_control_projection project_candidate_control(
    const package_source_record& source)
{
  std::vector<pkgplan::runtime_dependency_declaration> dependencies;
  for (const package_requirement& requirement : source.runtime_requirements())
  {
    dependencies.push_back(pkgplan::runtime_dependency_declaration::make(
        requirement.package().name()));
  }

  std::vector<pkgplan::removal_lifecycle_declaration> lifecycle;
  for (const lifecycle_action action : {
           lifecycle_action::pre_remove,
           lifecycle_action::post_remove})
  {
    const lifecycle_program* value = source.lifecycle(action);
    if (value == nullptr)
      continue;
    lifecycle.push_back(pkgplan::removal_lifecycle_declaration::make(
        planner_phase(action), "text/x-posix-shell",
        value->value().material()));
  }

  std::vector<pkgplan::target_profile_fact> target_profile;
  target_profile.push_back(pkgplan::target_profile_fact::make(
      "pkgsource.target-architectures",
      architecture_value(source.architectures().declared_target())));
  return pkgplan::candidate_control_projection(
      std::move(dependencies), std::move(lifecycle),
      std::move(target_profile));
}

void validate_incoming_release(
    const package_source_record& source,
    const pkgplan::package_release& planned,
    const pkgplan::candidate_control_projection& planned_control)
{
  const package_release& release = source.release();
  if (!same_identity_text(release.identity(), planned.identity()) ||
      release.name() != planned.name() ||
      release.version() != planned.version() ||
      std::to_string(release.release()) != planned.release())
  {
    throw projection_error(
        projection_error_code::incoming_authority_mismatch,
        "sealed source release does not match accepted plan");
  }

  try
  {
    if (project_candidate_control(source) != planned_control)
    {
      throw projection_error(
          projection_error_code::incoming_authority_mismatch,
          "sealed source control does not match accepted candidate control");
    }
  }
  catch (const projection_error&)
  {
    throw;
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::incoming_authority_mismatch,
        std::string("cannot compare sealed source with planner control: ") +
            error.what());
  }
}

const pkgapply::application_path_consequence* find_consequence(
    const std::vector<pkgapply::application_path_consequence>& consequences,
    const pkgplan::package_path& path)
{
  const auto found = std::lower_bound(
      consequences.begin(), consequences.end(), path,
      [](const auto& item, const pkgplan::package_path& wanted) {
        return item.path() < wanted;
      });
  if (found == consequences.end() || found->path() != path)
    return nullptr;
  return &*found;
}

owned_object_kind translate_object_kind(pkgapply::completed_object_kind kind)
{
  switch (kind)
  {
    case pkgapply::completed_object_kind::regular:
      return owned_object_kind::regular;
    case pkgapply::completed_object_kind::directory:
      return owned_object_kind::directory;
    case pkgapply::completed_object_kind::symlink:
      return owned_object_kind::symlink;
    case pkgapply::completed_object_kind::fifo:
      return owned_object_kind::fifo;
    case pkgapply::completed_object_kind::character_device:
      return owned_object_kind::character_device;
    case pkgapply::completed_object_kind::block_device:
      return owned_object_kind::block_device;
    case pkgapply::completed_object_kind::socket:
      return owned_object_kind::socket;
    case pkgapply::completed_object_kind::other:
      return owned_object_kind::other;
  }
  throw projection_error(projection_error_code::completed_path_mismatch,
                         "completed path has invalid object kind");
}


template<typename Value>
const Value& require_known(
    const pkgapply::qualified_fact<Value>& fact,
    const char* label)
{
  if (fact.state() != pkgapply::fact_state::known || !fact.value())
  {
    throw projection_error(
        projection_error_code::completed_path_mismatch,
        std::string("completed object lacks ") + label);
  }
  return *fact.value();
}

installed_object_metadata translate_object(
    const pkgapply::completed_object_fact& source)
{
  if (source.completeness() !=
      pkgapply::object_fact_completeness::complete)
  {
    throw projection_error(
        projection_error_code::completed_path_mismatch,
        "owned path object evidence is not complete");
  }

  const std::uint32_t mode = require_known(source.mode(), "mode");
  const std::uint64_t uid = require_known(source.uid(), "uid");
  const std::uint64_t gid = require_known(source.gid(), "gid");
  const pkgapply::completed_object_timestamp& source_mtime =
      require_known(source.mtime(), "mtime");
  installed_object_timestamp mtime(
      source_mtime.seconds, source_mtime.nanoseconds);

  std::optional<std::uint64_t> size;
  std::optional<installed_regular_content_identity> regular_content;
  std::optional<std::string> symlink_target;
  std::optional<installed_device_number> device;
  std::optional<package_path> hardlink_anchor;

  switch (source.kind())
  {
    case pkgapply::completed_object_kind::regular:
      size = require_known(source.size(), "regular size");
      regular_content =
          translate_identity<installed_regular_content_identity>(
              require_known(source.regular_content(),
                            "regular content identity"));
      if (source.hardlink().state() == pkgapply::fact_state::known)
      {
        hardlink_anchor = translate_path(
            require_known(source.hardlink(), "hard-link relation").anchor());
      }
      else if (source.hardlink().state() ==
               pkgapply::fact_state::not_applicable)
      {
        throw projection_error(
            projection_error_code::completed_path_mismatch,
            "regular object marks hard-link relation inapplicable");
      }
      break;

    case pkgapply::completed_object_kind::symlink:
      symlink_target = require_known(source.symlink_target(),
                                     "symlink target");
      break;

    case pkgapply::completed_object_kind::character_device:
    case pkgapply::completed_object_kind::block_device:
    {
      const pkgapply::completed_device_number& source_device =
          require_known(source.device(), "device number");
      device.emplace(source_device.major, source_device.minor);
      break;
    }

    case pkgapply::completed_object_kind::directory:
    case pkgapply::completed_object_kind::fifo:
    case pkgapply::completed_object_kind::socket:
    case pkgapply::completed_object_kind::other:
      break;
  }

  return installed_object_metadata(
      translate_object_kind(source.kind()), mode, uid, gid, std::move(mtime),
      std::move(size), std::move(regular_content),
      std::move(symlink_target), std::move(device),
      std::move(hardlink_anchor));
}

active_object_origin translate_origin(pkgplan::planned_active_outcome outcome)
{
  switch (outcome)
  {
    case pkgplan::planned_active_outcome::activate_incoming:
      return active_object_origin::incoming_payload;
    case pkgplan::planned_active_outcome::retain_observed:
      return active_object_origin::retained_existing;
    case pkgplan::planned_active_outcome::remove_observed:
    case pkgplan::planned_active_outcome::remove_directory_if_empty:
    case pkgplan::planned_active_outcome::remain_absent:
      break;
  }
  throw projection_error(
      projection_error_code::completed_path_mismatch,
      "owned path has no active object suitable for publication");
}

std::optional<rejected_object_reference> translate_rejected(
    const pkgapply::application_path_consequence& consequence)
{
  if (!consequence.rejected_object())
  {
    if (consequence.requested_rejected() !=
        pkgplan::planned_rejected_outcome::none)
    {
      throw projection_error(
          projection_error_code::completed_path_mismatch,
          "planned rejected object lacks completed durable identity");
    }
    return std::nullopt;
  }

  rejected_object_side side = rejected_object_side::incoming;
  switch (consequence.requested_rejected())
  {
    case pkgplan::planned_rejected_outcome::stage_incoming:
      side = rejected_object_side::incoming;
      break;
    case pkgplan::planned_rejected_outcome::stage_old:
      side = rejected_object_side::prior_installed;
      break;
    case pkgplan::planned_rejected_outcome::none:
      throw projection_error(
          projection_error_code::completed_path_mismatch,
          "unplanned rejected object appears in completed evidence");
  }

  return rejected_object_reference(
      side, translate_identity<rejected_object_identity>(
                *consequence.rejected_object()));
}

std::vector<owned_entry> translate_manifest(
    const std::vector<pkgplan::package_path>& planned,
    const pkgapply::completed_application_evidence& evidence)
{
  std::vector<owned_entry> manifest;
  manifest.reserve(planned.size());

  for (const pkgplan::package_path& path : planned)
  {
    const pkgapply::application_path_consequence* consequence =
        find_consequence(evidence.paths(), path);
    if (consequence == nullptr ||
        consequence->publication() !=
            pkgapply::ownership_publication_status::eligible ||
        !consequence->ownership().incoming_package_owns_after() ||
        consequence->after().state() != pkgapply::fact_state::known ||
        !consequence->after().object())
    {
      throw projection_error(
          projection_error_code::completed_path_mismatch,
          "planned installed path lacks publication-eligible completed truth");
    }

    manifest.push_back(owned_entry::make(
        translate_path(path),
        translate_object(*consequence->after().object()),
        translate_origin(consequence->requested_active()),
        translate_rejected(*consequence)));
  }

  for (const pkgapply::application_path_consequence& consequence :
       evidence.paths())
  {
    if (!consequence.ownership().incoming_package_owns_after())
      continue;
    if (!std::binary_search(planned.begin(), planned.end(),
                            consequence.path()))
    {
      throw projection_error(
          projection_error_code::completed_path_mismatch,
          "completed application grants unplanned incoming ownership");
    }
  }
  return manifest;
}

const installed_package& require_old_package(
    const snapshot& expected_state,
    const pkgplan::package_release& release,
    const pkgplan::installed_package_identity& package,
    const pkgplan::installed_control_identity& control)
{
  const installed_package* installed =
      expected_state.find_package(release.name());
  if (installed == nullptr ||
      !same_identity_text(installed->identity(), package) ||
      !same_identity_text(installed->control().identity(), control) ||
      !same_identity_text(installed->release().identity(), release.identity()) ||
      installed->release().name() != release.name() ||
      installed->release().version() != release.version() ||
      std::to_string(installed->release().release()) != release.release())
  {
    throw projection_error(
        projection_error_code::package_state_mismatch,
        "accepted plan does not name canonical installed package");
  }
  return *installed;
}

template<typename Publication>
installed_package construct_installed_package(
    const snapshot& expected_state,
    const Publication& publication,
    const pkgapply::completed_application_evidence& evidence,
    const build_adapter::build_authority& incoming,
    installation_reason reason,
    std::optional<transaction_evidence_identity> transaction_evidence)
{
  validate_incoming_release(incoming.source(), publication.release(),
                            publication.installed_control());

  if (publication.artifact().string() !=
      incoming.provenance().artifact_content().string())
  {
    throw projection_error(
        projection_error_code::incoming_authority_mismatch,
        "accepted plan artifact differs from admitted build bytes");
  }

  installed_control control = installed_control::make(
      incoming.source(), std::move(reason), incoming.provenance());
  std::vector<owned_entry> manifest =
      translate_manifest(publication.installed_manifest(), evidence);

  try
  {
    installation_receipt receipt = installation_receipt::make(
        std::move(control), expected_state.target_binding(),
        std::move(manifest),
        translate_identity<operation_plan_identity>(evidence.plan()),
        translate_identity<application_evidence_identity>(evidence.identity()),
        std::move(transaction_evidence));
    return installed_package::make(std::move(receipt));
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::publication_construction,
        std::string("state rejected resulting installation receipt: ") +
            error.what());
  }
}

build_adapter::build_authority admit_incoming_build(
    const pkgapply::incoming_package_authority& incoming)
{
  try
  {
    const pkgbuild::build_request& request = incoming.build().request();
    package_source_record source = source_adapter::project_source(
        request.source(), request.architectures().build(),
        request.architectures().target());
    return build_adapter::project_build(
        source, incoming.build(), incoming.image());
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::incoming_authority_mismatch,
        std::string("cannot admit request-bound source/build authority: ") +
            error.what());
  }
}

state_publication_request construct_installation(
    const snapshot& expected_state,
    const pkgapply::installation_application_request& request,
    const pkgapply::completed_application_evidence& evidence,
    installation_reason reason,
    std::optional<transaction_evidence_identity> transaction_evidence)
{
  build_adapter::build_authority incoming =
      admit_incoming_build(request.incoming());

  const auto& publication = request.plan().publication();
  if (expected_state.find_package(publication.release().name()) != nullptr)
  {
    throw projection_error(projection_error_code::package_state_mismatch,
                           "installation package is already present in state");
  }

  installed_package proposed = construct_installed_package(
      expected_state, publication, evidence, incoming, std::move(reason),
      transaction_evidence);
  package_state_delta delta = package_state_delta::install(
      std::move(proposed),
      translate_identity<operation_plan_identity>(request.plan().identity()),
      translate_identity<application_evidence_identity>(evidence.identity()));
  return state_publication_request::make(
      expected_state, {std::move(delta)}, std::move(transaction_evidence));
}

state_publication_request construct_upgrade(
    const snapshot& expected_state,
    const pkgapply::upgrade_application_request& request,
    const pkgapply::completed_application_evidence& evidence,
    std::optional<transaction_evidence_identity> transaction_evidence)
{
  build_adapter::build_authority incoming =
      admit_incoming_build(request.incoming());

  const auto& plan = request.plan();
  const auto& publication = plan.publication();
  const installed_package& old = require_old_package(
      expected_state, plan.old_release(), publication.replaced_package(),
      publication.replaced_control());

  installed_package proposed = construct_installed_package(
      expected_state, publication, evidence, incoming,
      old.control().reason(), transaction_evidence);
  package_state_delta delta = package_state_delta::replace(
      old.identity(), std::move(proposed),
      translate_identity<operation_plan_identity>(plan.identity()),
      translate_identity<application_evidence_identity>(evidence.identity()));
  return state_publication_request::make(
      expected_state, {std::move(delta)}, std::move(transaction_evidence));
}

state_publication_request construct_removal(
    const snapshot& expected_state,
    const pkgapply::removal_application_request& request,
    const pkgapply::completed_application_evidence& evidence,
    std::optional<transaction_evidence_identity> transaction_evidence)
{
  const auto& plan = request.plan();
  const auto& publication = plan.publication();
  const installed_package& old = require_old_package(
      expected_state, plan.release(), publication.removed_package(),
      publication.removed_control());

  package_state_delta delta = package_state_delta::remove(
      old.release().name(), old.identity(),
      translate_identity<operation_plan_identity>(plan.identity()),
      translate_identity<application_evidence_identity>(evidence.identity()));
  return state_publication_request::make(
      expected_state, {std::move(delta)}, std::move(transaction_evidence));
}

} // namespace

projection_error::projection_error(projection_error_code code,
                                   std::string message)
    : std::invalid_argument(std::move(message)), code_(code)
{
}
projection_error_code projection_error::code() const noexcept { return code_; }

state_publication_request project_completed_application(
    const snapshot& expected_state,
    const pkgapply::lease_bound_state_projection& application_state,
    const pkgapply::installation_application_request& request,
    const pkgapply::completed_application_evidence& evidence,
    installation_reason reason)
{
  const pkgapply::package_application_request envelope(request);
  validate_common(expected_state, application_state, envelope, evidence);
  try
  {
    return construct_installation(
        expected_state, request, evidence, std::move(reason), std::nullopt);
  }
  catch (const projection_error&)
  {
    throw;
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::publication_construction,
        std::string("state rejected installation publication projection: ") +
            error.what());
  }
}

state_publication_request project_completed_application(
    const snapshot& expected_state,
    const pkgapply::lease_bound_state_projection& application_state,
    const pkgapply::installation_application_request& request,
    const pkgapply::completed_application_evidence& evidence,
    installation_reason reason,
    transaction_evidence_identity transaction_evidence)
{
  const pkgapply::package_application_request envelope(request);
  validate_common(expected_state, application_state, envelope, evidence);
  try
  {
    return construct_installation(
        expected_state, request, evidence, std::move(reason),
        std::move(transaction_evidence));
  }
  catch (const projection_error&)
  {
    throw;
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::publication_construction,
        std::string("state rejected installation publication projection: ") +
            error.what());
  }
}

state_publication_request project_completed_application(
    const snapshot& expected_state,
    const pkgapply::lease_bound_state_projection& application_state,
    const pkgapply::upgrade_application_request& request,
    const pkgapply::completed_application_evidence& evidence)
{
  const pkgapply::package_application_request envelope(request);
  validate_common(expected_state, application_state, envelope, evidence);
  try
  {
    return construct_upgrade(
        expected_state, request, evidence, std::nullopt);
  }
  catch (const projection_error&)
  {
    throw;
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::publication_construction,
        std::string("state rejected upgrade publication projection: ") +
            error.what());
  }
}

state_publication_request project_completed_application(
    const snapshot& expected_state,
    const pkgapply::lease_bound_state_projection& application_state,
    const pkgapply::upgrade_application_request& request,
    const pkgapply::completed_application_evidence& evidence,
    transaction_evidence_identity transaction_evidence)
{
  const pkgapply::package_application_request envelope(request);
  validate_common(expected_state, application_state, envelope, evidence);
  try
  {
    return construct_upgrade(
        expected_state, request, evidence, std::move(transaction_evidence));
  }
  catch (const projection_error&)
  {
    throw;
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::publication_construction,
        std::string("state rejected upgrade publication projection: ") +
            error.what());
  }
}

state_publication_request project_completed_application(
    const snapshot& expected_state,
    const pkgapply::lease_bound_state_projection& application_state,
    const pkgapply::removal_application_request& request,
    const pkgapply::completed_application_evidence& evidence)
{
  const pkgapply::package_application_request envelope(request);
  validate_common(expected_state, application_state, envelope, evidence);
  try
  {
    return construct_removal(
        expected_state, request, evidence, std::nullopt);
  }
  catch (const projection_error&)
  {
    throw;
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::publication_construction,
        std::string("state rejected removal publication projection: ") +
            error.what());
  }
}

state_publication_request project_completed_application(
    const snapshot& expected_state,
    const pkgapply::lease_bound_state_projection& application_state,
    const pkgapply::removal_application_request& request,
    const pkgapply::completed_application_evidence& evidence,
    transaction_evidence_identity transaction_evidence)
{
  const pkgapply::package_application_request envelope(request);
  validate_common(expected_state, application_state, envelope, evidence);
  try
  {
    return construct_removal(
        expected_state, request, evidence, std::move(transaction_evidence));
  }
  catch (const projection_error&)
  {
    throw;
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::publication_construction,
        std::string("state rejected removal publication projection: ") +
            error.what());
  }
}

} // namespace pkgstate::apply_adapter
