// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate-plan/adapter.h>

#include <libpkgplan/control.h>
#include <libpkgplan/fact_error.h>

#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace pkgstate::plan_adapter {
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
        std::string("planner identity vocabulary rejected canonical state: ") +
            error.what());
  }
}

pkgplan::package_path translate_path(const package_path& source)
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

pkgplan::filesystem_object_kind translate_kind(owned_object_kind source)
{
  switch (source)
  {
    case owned_object_kind::regular:
      return pkgplan::filesystem_object_kind::regular;
    case owned_object_kind::directory:
      return pkgplan::filesystem_object_kind::directory;
    case owned_object_kind::symlink:
      return pkgplan::filesystem_object_kind::symlink;
    case owned_object_kind::fifo:
      return pkgplan::filesystem_object_kind::fifo;
    case owned_object_kind::character_device:
      return pkgplan::filesystem_object_kind::character_device;
    case owned_object_kind::block_device:
      return pkgplan::filesystem_object_kind::block_device;
    case owned_object_kind::socket:
      return pkgplan::filesystem_object_kind::socket;
    case owned_object_kind::other:
      return pkgplan::filesystem_object_kind::other;
  }
  throw projection_error(projection_error_code::object_translation,
                         "invalid installed object kind");
}

pkgplan::filesystem_object_metadata translate_object(
    const installed_object_metadata& source)
{
  try
  {
    std::optional<pkgplan::object_timestamp> mtime;
    mtime.emplace(source.mtime().seconds(), source.mtime().nanoseconds());

    std::optional<pkgplan::filesystem_regular_content_identity> content;
    if (source.regular_content())
    {
      content = translate_identity<
          pkgplan::filesystem_regular_content_identity>(
              *source.regular_content());
    }

    std::optional<pkgplan::device_number> device;
    if (source.device())
      device.emplace(source.device()->major(), source.device()->minor());

    return pkgplan::filesystem_object_metadata(
        translate_kind(source.kind()), source.mode(), source.uid(),
        source.gid(), source.size(), std::move(mtime), std::move(content),
        source.symlink_target(), std::move(device));
  }
  catch (const projection_error&)
  {
    throw;
  }
  catch (const pkgplan::fact_error& error)
  {
    throw projection_error(
        projection_error_code::object_translation,
        std::string("planner object vocabulary rejected canonical state: ") +
            error.what());
  }
}

pkgplan::removal_lifecycle_phase translate_phase(lifecycle_action source)
{
  switch (source)
  {
    case lifecycle_action::pre_remove:
      return pkgplan::removal_lifecycle_phase::pre_remove;
    case lifecycle_action::post_remove:
      return pkgplan::removal_lifecycle_phase::post_remove;
    case lifecycle_action::pre_install:
    case lifecycle_action::post_install:
      break;
  }
  throw projection_error(projection_error_code::control_translation,
                         "installation lifecycle is not planner removal control");
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

pkgplan::installed_control_projection translate_control(
    const installed_control& control)
{
  try
  {
    const package_source_record& source = control.source();

    std::vector<pkgplan::runtime_dependency_declaration> dependencies;
    dependencies.reserve(source.runtime_requirements().size());
    for (const package_requirement& dependency : source.runtime_requirements())
    {
      dependencies.push_back(pkgplan::runtime_dependency_declaration::make(
          dependency.package().name()));
    }

    std::vector<pkgplan::removal_lifecycle_declaration> lifecycle;
    for (const lifecycle_action action : {
             lifecycle_action::pre_remove,
             lifecycle_action::post_remove})
    {
      const lifecycle_program* declaration = source.lifecycle(action);
      if (declaration == nullptr)
        continue;
      lifecycle.push_back(pkgplan::removal_lifecycle_declaration::make(
          translate_phase(action), "text/x-posix-shell",
          declaration->value().material()));
    }

    std::vector<pkgplan::target_profile_fact> target_profile;
    target_profile.push_back(pkgplan::target_profile_fact::make(
        "pkgsource.target-architectures",
        architecture_value(source.architectures().declared_target())));

    pkgplan::installed_control_completeness completeness;
    completeness.runtime_dependencies = pkgplan::control_fact_availability::known;
    completeness.removal_lifecycle = pkgplan::control_fact_availability::known;
    completeness.target_profile = pkgplan::control_fact_availability::known;

    return pkgplan::installed_control_projection(
        completeness, std::move(dependencies), std::move(lifecycle),
        std::move(target_profile));
  }
  catch (const projection_error&)
  {
    throw;
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

projection_error_code projection_error::code() const noexcept { return code_; }

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
    : target_(std::move(target)), packages_(std::move(packages)),
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

installed_state_projection project_installed_state(
    const snapshot& state, const planning_target_context& target)
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
    claim_count += package.manifest().size();
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
        planner_release_identity, package.release().name(),
        package.release().version(),
        std::to_string(package.release().release()));
    auto control_projection = translate_control(package.control());
    packages.emplace_back(
        planner_package, planner_control, planner_snapshot,
        std::move(release), std::move(control_projection));

    for (const owned_entry& entry : package.manifest())
    {
      claims.emplace_back(translate_path(entry.path()), planner_package,
                          translate_object(entry.object()));
    }
  }

  pkgplan::installed_ownership_inventory ownership(
      planner_ownership, planner_snapshot,
      pkgplan::fact_set_completeness::complete, std::move(claims));

  return installed_state_projection(target.identity(), std::move(packages),
                                    std::move(ownership));
}

} // namespace pkgstate::plan_adapter
