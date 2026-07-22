// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/snapshot.h>

#include <algorithm>
#include <utility>

#include <libpkgstate/error.h>

namespace pkgstate {

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
      owners[entry.path.string()].push_back(index);
  }

  return snapshot(std::move(target_binding),
                  std::move(packages),
                  std::move(owners));
}

snapshot::snapshot(
    state_target_binding target_binding,
    std::vector<installed_package> packages,
    std::unordered_map<std::string, std::vector<std::size_t>> owners)
    : target_binding_(std::move(target_binding)),
      packages_(std::move(packages)),
      owners_(std::move(owners))
{
}

std::uint16_t
snapshot::schema_version() const noexcept
{
  return installed_state_schema_version;
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
