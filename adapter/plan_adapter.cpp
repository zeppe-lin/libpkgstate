// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate-plan/adapter.h>

#include <libpkgplan/fact_error.h>

#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace pkgstate::plan_adapter {
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
        std::string("planner identity vocabulary rejected canonical state: ") +
            error.what());
  }
}

pkgplan::package_path
translate_path(const package_path& source)
{
  try
  {
    return pkgplan::package_path::parse(source.string());
  }
  catch (const std::exception& error)
  {
    throw projection_error(
        projection_error_code::path_translation,
        std::string("planner path vocabulary rejected canonical state: ") +
            error.what());
  }
}

pkgplan::control_fact_availability
translate_availability(installed_control_fact_state source)
{
  switch (source)
  {
    case installed_control_fact_state::recorded_at_installation:
    case installed_control_fact_state::recorded_in_compatibility_storage:
    case installed_control_fact_state::supplied_by_migration:
      return pkgplan::control_fact_availability::known;
    case installed_control_fact_state::historically_unavailable:
      return pkgplan::control_fact_availability::historically_unavailable;
  }

  throw projection_error(
      projection_error_code::control_translation,
      "installed control contains an invalid fact-state value");
}

pkgplan::removal_lifecycle_phase
translate_phase(removal_lifecycle_phase source)
{
  switch (source)
  {
    case removal_lifecycle_phase::pre_remove:
      return pkgplan::removal_lifecycle_phase::pre_remove;
    case removal_lifecycle_phase::post_remove:
      return pkgplan::removal_lifecycle_phase::post_remove;
  }

  throw projection_error(
      projection_error_code::control_translation,
      "installed control contains an invalid lifecycle phase");
}

pkgplan::installed_control_projection
translate_control(const installed_control& source)
{
  try
  {
    std::vector<pkgplan::runtime_dependency_declaration> dependencies;
    dependencies.reserve(source.runtime_dependencies().size());
    for (const runtime_dependency_declaration& dependency :
         source.runtime_dependencies())
    {
      dependencies.push_back(
          pkgplan::runtime_dependency_declaration::make(
              dependency.expression()));
    }

    std::vector<pkgplan::removal_lifecycle_declaration> lifecycle;
    lifecycle.reserve(source.removal_lifecycle().size());
    for (const removal_lifecycle_declaration& declaration :
         source.removal_lifecycle())
    {
      lifecycle.push_back(
          pkgplan::removal_lifecycle_declaration::make(
              translate_phase(declaration.phase()),
              declaration.format(),
              declaration.material()));
    }

    std::vector<pkgplan::target_profile_fact> profile;
    profile.reserve(source.target_profile().size());
    for (const target_profile_fact& fact : source.target_profile())
    {
      profile.push_back(
          pkgplan::target_profile_fact::make(fact.name(), fact.value()));
    }

    const installed_control_completeness& completeness = source.completeness();
    pkgplan::installed_control_completeness projected_completeness;
    projected_completeness.runtime_dependencies =
        translate_availability(completeness.runtime_dependencies);
    projected_completeness.removal_lifecycle =
        translate_availability(completeness.removal_lifecycle);
    projected_completeness.target_profile =
        translate_availability(completeness.target_profile);

    return pkgplan::installed_control_projection(
        projected_completeness,
        std::move(dependencies),
        std::move(lifecycle),
        std::move(profile));
  }
  catch (const pkgplan::fact_error& error)
  {
    throw projection_error(
        projection_error_code::control_translation,
        std::string("planner control vocabulary rejected canonical state: ") +
            error.what());
  }
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

planning_target_context::planning_target_context(
    pkgplan::target_system_context_identity identity,
    state_target_binding state_projection)
    : identity_(std::move(identity)),
      state_projection_(std::move(state_projection))
{
}

const pkgplan::target_system_context_identity&
planning_target_context::identity() const noexcept
{
  return identity_;
}

const state_target_binding&
planning_target_context::state_projection() const noexcept
{
  return state_projection_;
}

installed_state_projection::installed_state_projection(
    pkgplan::target_system_context_identity target,
    std::vector<pkgplan::installed_package_fact> packages,
    pkgplan::installed_ownership_inventory ownership)
    : target_(std::move(target)),
      packages_(std::move(packages)),
      ownership_(std::move(ownership))
{
}

const pkgplan::target_system_context_identity&
installed_state_projection::target() const noexcept
{
  return target_;
}

const std::vector<pkgplan::installed_package_fact>&
installed_state_projection::packages() const noexcept
{
  return packages_;
}

const pkgplan::installed_ownership_inventory&
installed_state_projection::ownership() const noexcept
{
  return ownership_;
}

installed_state_projection
project_installed_state(const snapshot& state,
                        const planning_target_context& target)
{
  if (target.state_projection() != state.target_binding())
  {
    throw projection_error(
        projection_error_code::target_binding_mismatch,
        "planner target state projection does not match installed snapshot");
  }

  const auto planner_snapshot =
      translate_identity<pkgplan::installed_state_snapshot_identity>(
          state.identity());
  const auto planner_ownership =
      translate_identity<pkgplan::ownership_inventory_identity>(
          state.ownership_identity());

  std::vector<pkgplan::installed_package_fact> packages;
  std::vector<pkgplan::installed_ownership_claim> claims;
  packages.reserve(state.packages().size());

  std::size_t claim_count = 0;
  for (const installed_package& package : state.packages())
  {
    claim_count += package.manifest().size();
  }
  claims.reserve(claim_count);

  for (const installed_package& package : state.packages())
  {
    const auto planner_package =
        translate_identity<pkgplan::installed_package_identity>(
            package.identity());
    const auto planner_control =
        translate_identity<pkgplan::installed_control_identity>(
            package.control().identity());
    const auto planner_release_identity =
        translate_identity<pkgplan::package_release_identity>(
            package.release().identity());

    pkgplan::package_release release(
        planner_release_identity,
        package.release().name(),
        package.release().version(),
        package.release().release());
    auto control_projection = translate_control(package.control());
    packages.emplace_back(
        planner_package,
        planner_control,
        planner_snapshot,
        std::move(release),
        std::move(control_projection));

    for (const owned_entry& entry : package.manifest())
    {
      claims.emplace_back(
          translate_path(entry.path), planner_package, std::nullopt);
    }
  }

  pkgplan::installed_ownership_inventory ownership(
      planner_ownership,
      planner_snapshot,
      pkgplan::fact_set_completeness::complete,
      std::move(claims));

  return installed_state_projection(
      target.identity(), std::move(packages), std::move(ownership));
}

} // namespace pkgstate::plan_adapter
