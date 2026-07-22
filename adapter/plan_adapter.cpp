// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate-plan/adapter.h>

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
    packages.emplace_back(
        planner_package, planner_control, planner_snapshot, std::move(release));

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
