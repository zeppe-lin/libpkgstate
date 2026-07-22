// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/legacy_snapshot.h>

#include <algorithm>
#include <utility>

#include <libpkgstate/error.h>

namespace pkgstate {

legacy_snapshot::legacy_snapshot(
    std::vector<legacy_installed_package> packages)
    : packages_(std::move(packages))
{
  std::sort(packages_.begin(), packages_.end(),
            [](const legacy_installed_package& lhs,
               const legacy_installed_package& rhs) {
              return lhs.identity().name() < rhs.identity().name();
            });

  for (std::size_t index = 0; index < packages_.size(); ++index)
  {
    if (index != 0 &&
        packages_[index - 1].identity().name() ==
            packages_[index].identity().name())
    {
      throw state_error("duplicate legacy installed package: " +
                        packages_[index].identity().name());
    }

    for (const owned_entry& entry : packages_[index].manifest())
      owners_[entry.path.string()].push_back(index);
  }
}

const std::vector<legacy_installed_package>&
legacy_snapshot::packages() const noexcept
{
  return packages_;
}

std::size_t
legacy_snapshot::size() const noexcept
{
  return packages_.size();
}

const legacy_installed_package*
legacy_snapshot::find_package(std::string_view name) const noexcept
{
  const auto found = std::lower_bound(
      packages_.begin(), packages_.end(), name,
      [](const legacy_installed_package& package,
         std::string_view searched) {
        return package.identity().name() < searched;
      });

  if (found == packages_.end() || found->identity().name() != name)
    return nullptr;

  return &*found;
}

std::vector<const legacy_installed_package*>
legacy_snapshot::owners(const package_path& path) const
{
  std::vector<const legacy_installed_package*> result;
  const auto found = owners_.find(path.string());
  if (found == owners_.end())
    return result;

  result.reserve(found->second.size());
  for (const std::size_t index : found->second)
    result.push_back(&packages_[index]);

  return result;
}

bool
legacy_snapshot::is_owned(const package_path& path) const noexcept
{
  return owners_.find(path.string()) != owners_.end();
}

} // namespace pkgstate
