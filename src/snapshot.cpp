// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/snapshot.h>

#include "canonical_record.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <libpkgstate/error.h>

namespace pkgstate {
namespace {

using canonical_ownership =
    std::map<std::string, std::vector<installed_package_identity>>;

ownership_inventory_identity
identify_ownership(canonical_ownership& ownership)
{
  detail::canonical_record record(
      ownership_inventory_identity::canonical_domain());
  record.append_u64(static_cast<std::uint64_t>(ownership.size()));

  for (auto& [path, owners] : ownership)
  {
    std::sort(owners.begin(), owners.end());
    const auto duplicate = std::adjacent_find(owners.begin(), owners.end());
    if (duplicate != owners.end())
      throw state_error("duplicate ownership claim for path: " + path);

    record.append_bytes(path);
    record.append_u64(static_cast<std::uint64_t>(owners.size()));
    for (const installed_package_identity& owner : owners)
      record.append_digest(owner);
  }

  return ownership_inventory_identity::from_sha256(record.sha256());
}

installed_state_snapshot_identity
identify_snapshot(const state_target_binding& target_binding,
                  const std::vector<installed_package>& packages,
                  const ownership_inventory_identity& ownership_identity)
{
  detail::canonical_record record(
      installed_state_snapshot_identity::canonical_domain());
  record.append_u16(installed_state_schema_version);
  record.append_digest(target_binding.identity());
  record.append_u64(static_cast<std::uint64_t>(packages.size()));
  for (const installed_package& package : packages)
    record.append_digest(package.identity());
  record.append_digest(ownership_identity);

  return installed_state_snapshot_identity::from_sha256(record.sha256());
}

} // namespace

snapshot
snapshot::make(state_target_binding target_binding,
               std::vector<installed_package> packages)
{
  std::sort(packages.begin(), packages.end(),
            [](const installed_package& lhs,
               const installed_package& rhs) {
              return lhs.release().name() < rhs.release().name();
            });

  std::unordered_map<std::string, std::vector<std::size_t>> owners;
  canonical_ownership canonical_owners;
  for (std::size_t index = 0; index < packages.size(); ++index)
  {
    const installed_package& package = packages[index];
    if (package.target_binding() != target_binding)
    {
      throw state_error("installed package belongs to another target: " +
                        package.release().name());
    }

    if (index != 0 &&
        packages[index - 1].release().name() == package.release().name())
    {
      throw state_error("duplicate installed package: " +
                        package.release().name());
    }

    for (const owned_entry& entry : package.manifest())
    {
      owners[entry.path.string()].push_back(index);
      canonical_owners[entry.path.string()].push_back(package.identity());
    }
  }

  ownership_inventory_identity ownership_identity =
      identify_ownership(canonical_owners);
  installed_state_snapshot_identity identity =
      identify_snapshot(target_binding, packages, ownership_identity);

  return snapshot(std::move(identity),
                  std::move(ownership_identity),
                  std::move(target_binding),
                  std::move(packages),
                  std::move(owners));
}

snapshot::snapshot(
    installed_state_snapshot_identity identity,
    ownership_inventory_identity ownership_identity,
    state_target_binding target_binding,
    std::vector<installed_package> packages,
    std::unordered_map<std::string, std::vector<std::size_t>> owners)
    : identity_(std::move(identity)),
      ownership_identity_(std::move(ownership_identity)),
      target_binding_(std::move(target_binding)),
      packages_(std::move(packages)),
      owners_(std::move(owners))
{
}

std::uint16_t
snapshot::schema_version() const noexcept
{
  return installed_state_schema_version;
}

const ownership_inventory_identity&
snapshot::ownership_identity() const noexcept
{
  return ownership_identity_;
}

const installed_state_snapshot_identity&
snapshot::identity() const noexcept
{
  return identity_;
}

const state_target_binding&
snapshot::target_binding() const noexcept
{
  return target_binding_;
}

const std::vector<installed_package>&
snapshot::packages() const noexcept
{
  return packages_;
}

std::size_t
snapshot::size() const noexcept
{
  return packages_.size();
}

const installed_package*
snapshot::find_package(std::string_view name) const noexcept
{
  const auto found = std::lower_bound(
      packages_.begin(), packages_.end(), name,
      [](const installed_package& package, std::string_view searched) {
        return package.release().name() < searched;
      });

  if (found == packages_.end() || found->release().name() != name)
    return nullptr;

  return &*found;
}

std::vector<const installed_package*>
snapshot::owners(const package_path& path) const
{
  std::vector<const installed_package*> result;
  const auto found = owners_.find(path.string());
  if (found == owners_.end())
    return result;

  result.reserve(found->second.size());
  for (const std::size_t index : found->second)
    result.push_back(&packages_[index]);

  return result;
}

bool
snapshot::is_owned(const package_path& path) const noexcept
{
  return owners_.find(path.string()) != owners_.end();
}

} // namespace pkgstate
