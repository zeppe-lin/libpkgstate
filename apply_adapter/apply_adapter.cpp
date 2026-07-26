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
#include <libpkgstate/installed_control.h>
#include <libpkgstate/installed_package.h>
#include <libpkgstate/owned_entry.h>
#include <libpkgstate/package_path.h>
#include <libpkgstate/package_release.h>

namespace pkgstate::apply_adapter {
namespace {

template<typename Destination, typename Source>
Destination
translate_identity(const Source& source)
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

package_path
translate_path(const pkgplan::package_path& source)
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
bool
same_identity_text(const Left& lhs, const Right& rhs)
{
  return lhs.string() == rhs.string();
}

const pkgplan::operation_preconditions&
preconditions(const pkgapply::package_application_request& request)
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

void
validate_common(
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
    throw projection_error(
        projection_error_code::expected_state_mismatch,
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
        "application projection does not satisfy the accepted plan authorities");
  }

  if (!same_identity_text(application_state.snapshot(),
                          expected_state.identity()) ||
      !same_identity_text(application_state.ownership_inventory(),
                          expected_state.ownership_identity()))
  {
    throw projection_error(
        projection_error_code::expected_state_mismatch,
        "application projection does not name the canonical expected state");
  }

  const state_target_binding& state_target = expected_state.target_binding();
  if (!same_identity_text(request.target().managed_target(),
                          state_target.managed_target()) ||
      !same_identity_text(request.target().root_view(),
                          state_target.root_view()))
  {
    throw projection_error(
        projection_error_code::target_binding_mismatch,
        "application target does not match the canonical state target");
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
          "completed path is absent from the lease-bound state projection");
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
          "lease-projected path owners differ from canonical installed state");
    }
  }
}

package_release
translate_release(const pkgplan::package_release& source)
{
  try
  {
    return package_release::make(
        source.name(), source.version(), source.release());
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::control_translation,
        std::string("state release vocabulary rejected planner release: ") +
            error.what());
  }
}

removal_lifecycle_phase
translate_phase(pkgplan::removal_lifecycle_phase source)
{
  switch (source)
  {
    case pkgplan::removal_lifecycle_phase::pre_remove:
      return removal_lifecycle_phase::pre_remove;
    case pkgplan::removal_lifecycle_phase::post_remove:
      return removal_lifecycle_phase::post_remove;
  }
  throw projection_error(projection_error_code::control_translation,
                         "invalid planner removal lifecycle phase");
}

installed_control
translate_control(
    const package_release& release,
    const pkgplan::candidate_control_projection& source,
    const pkgplan::candidate_control_identity& candidate,
    const pkgplan::artifact_identity& artifact,
    const pkgplan::artifact_manifest_identity& artifact_manifest,
    const pkgapply::completed_application_evidence_identity& evidence)
{
  try
  {
    std::vector<runtime_dependency_declaration> dependencies;
    dependencies.reserve(source.runtime_dependencies().size());
    for (const auto& declaration : source.runtime_dependencies())
    {
      dependencies.push_back(
          runtime_dependency_declaration::make(declaration.expression()));
    }

    std::vector<removal_lifecycle_declaration> lifecycle;
    lifecycle.reserve(source.removal_lifecycle().size());
    for (const auto& declaration : source.removal_lifecycle())
    {
      lifecycle.push_back(removal_lifecycle_declaration::make(
          translate_phase(declaration.phase()),
          declaration.format(),
          declaration.material()));
    }

    std::vector<target_profile_fact> profile;
    profile.reserve(source.target_profile().size());
    for (const auto& fact : source.target_profile())
    {
      profile.push_back(target_profile_fact::make(fact.name(), fact.value()));
    }

    std::vector<control_provenance> provenance;
    provenance.push_back(control_provenance::make(
        control_provenance_kind::candidate_control, candidate.string()));
    provenance.push_back(control_provenance::make(
        control_provenance_kind::artifact, artifact.string()));
    provenance.push_back(control_provenance::make(
        control_provenance_kind::artifact_manifest,
        artifact_manifest.string()));
    provenance.push_back(control_provenance::make(
        control_provenance_kind::application_evidence, evidence.string()));

    installed_control_completeness completeness;
    completeness.runtime_dependencies =
        installed_control_fact_state::recorded_at_installation;
    completeness.removal_lifecycle =
        installed_control_fact_state::recorded_at_installation;
    completeness.target_profile =
        installed_control_fact_state::recorded_at_installation;
    completeness.provenance =
        installed_control_fact_state::recorded_at_installation;

    return installed_control::make(
        release,
        completeness,
        std::move(dependencies),
        std::move(lifecycle),
        std::move(profile),
        std::move(provenance));
  }
  catch (const projection_error&)
  {
    throw;
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::control_translation,
        std::string("state control vocabulary rejected planner control: ") +
            error.what());
  }
}

const pkgapply::application_path_consequence*
find_consequence(
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

owned_entry_type
translate_owned_type(pkgapply::completed_object_kind kind)
{
  return kind == pkgapply::completed_object_kind::directory
      ? owned_entry_type::directory
      : owned_entry_type::non_directory;
}

std::vector<owned_entry>
translate_manifest(
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
        !consequence->after().object().has_value())
    {
      throw projection_error(
          projection_error_code::completed_path_mismatch,
          "planned installed path lacks publication-eligible completed truth");
    }

    manifest.push_back(owned_entry{
        translate_path(path),
        translate_owned_type(consequence->after().object()->kind())});
  }

  for (const pkgapply::application_path_consequence& consequence :
       evidence.paths())
  {
    if (!consequence.ownership().incoming_package_owns_after())
      continue;
    if (!std::binary_search(planned.begin(), planned.end(), consequence.path()))
    {
      throw projection_error(
          projection_error_code::completed_path_mismatch,
          "completed application grants unplanned incoming ownership");
    }
  }

  return manifest;
}

const installed_package&
require_old_package(const snapshot& expected_state,
                    const pkgplan::package_release& release,
                    const pkgplan::installed_package_identity& package,
                    const pkgplan::installed_control_identity& control)
{
  const installed_package* installed =
      expected_state.find_package(release.name());
  if (installed == nullptr ||
      !same_identity_text(installed->identity(), package) ||
      !same_identity_text(installed->control().identity(), control) ||
      installed->release().name() != release.name() ||
      installed->release().version() != release.version() ||
      installed->release().release() != release.release())
  {
    throw projection_error(
        projection_error_code::package_state_mismatch,
        "accepted plan does not name the canonical installed package");
  }
  return *installed;
}

template<typename Publication>
installed_package
construct_installed_package(
    const snapshot& expected_state,
    const Publication& publication,
    const pkgapply::completed_application_evidence& evidence)
{
  package_release release = translate_release(publication.release());
  installed_control control = translate_control(
      release,
      publication.installed_control(),
      publication.candidate(),
      publication.artifact(),
      publication.artifact_manifest(),
      evidence.identity());
  std::vector<owned_entry> manifest =
      translate_manifest(publication.installed_manifest(), evidence);

  try
  {
    return installed_package::make(
        std::move(release),
        std::move(control),
        expected_state.target_binding(),
        std::move(manifest));
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::publication_construction,
        std::string("state rejected resulting installed package: ") +
            error.what());
  }
}

state_publication_request
construct_installation(
    const snapshot& expected_state,
    const pkgapply::installation_application_request& request,
    const pkgapply::completed_application_evidence& evidence)
{
  const auto& publication = request.plan().publication();
  if (expected_state.find_package(publication.release().name()) != nullptr)
  {
    throw projection_error(projection_error_code::package_state_mismatch,
                           "installation package is already present in state");
  }

  installed_package proposed =
      construct_installed_package(expected_state, publication, evidence);
  package_state_delta delta = package_state_delta::install(
      std::move(proposed),
      translate_identity<operation_plan_identity>(request.plan().identity()),
      translate_identity<application_evidence_identity>(evidence.identity()));
  return state_publication_request::make(expected_state, {std::move(delta)});
}

state_publication_request
construct_upgrade(
    const snapshot& expected_state,
    const pkgapply::upgrade_application_request& request,
    const pkgapply::completed_application_evidence& evidence)
{
  const auto& plan = request.plan();
  const auto& publication = plan.publication();
  const installed_package& old = require_old_package(
      expected_state,
      plan.old_release(),
      publication.replaced_package(),
      publication.replaced_control());

  installed_package proposed =
      construct_installed_package(expected_state, publication, evidence);
  package_state_delta delta = package_state_delta::replace(
      old.identity(),
      std::move(proposed),
      translate_identity<operation_plan_identity>(plan.identity()),
      translate_identity<application_evidence_identity>(evidence.identity()));
  return state_publication_request::make(expected_state, {std::move(delta)});
}

state_publication_request
construct_removal(
    const snapshot& expected_state,
    const pkgapply::removal_application_request& request,
    const pkgapply::completed_application_evidence& evidence)
{
  const auto& plan = request.plan();
  const auto& publication = plan.publication();
  const installed_package& old = require_old_package(
      expected_state,
      plan.release(),
      publication.removed_package(),
      publication.removed_control());

  package_state_delta delta = package_state_delta::remove(
      old.release().name(),
      old.identity(),
      translate_identity<operation_plan_identity>(plan.identity()),
      translate_identity<application_evidence_identity>(evidence.identity()));
  return state_publication_request::make(expected_state, {std::move(delta)});
}

} // namespace

projection_error::projection_error(projection_error_code code,
                                   std::string message)
    : std::invalid_argument(std::move(message)), code_(code)
{
}

projection_error_code
projection_error::code() const noexcept
{
  return code_;
}

state_publication_request
project_completed_application(
    const snapshot& expected_state,
    const pkgapply::lease_bound_state_projection& application_state,
    const pkgapply::package_application_request& request,
    const pkgapply::completed_application_evidence& evidence)
{
  validate_common(expected_state, application_state, request, evidence);

  try
  {
    if (const auto* install = request.installation())
      return construct_installation(expected_state, *install, evidence);
    if (const auto* upgrade = request.upgrade())
      return construct_upgrade(expected_state, *upgrade, evidence);
    if (const auto* removal = request.removal())
      return construct_removal(expected_state, *removal, evidence);
  }
  catch (const projection_error&)
  {
    throw;
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::publication_construction,
        std::string("state rejected application publication projection: ") +
            error.what());
  }

  throw projection_error(projection_error_code::operation_binding_mismatch,
                         "application request has no supported operation body");
}

} // namespace pkgstate::apply_adapter
