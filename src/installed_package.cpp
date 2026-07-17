/*
 * Copyright (C) 2026 Alexandr Savca
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <libpkgstate/installed_package.h>

#include <algorithm>
#include <utility>

namespace pkgstate {

installed_package::installed_package(package_identity identity,
                                     std::vector<owned_entry> manifest)
    : identity_(std::move(identity)), manifest_(std::move(manifest))
{
  std::sort(manifest_.begin(), manifest_.end(),
            [](const owned_entry& lhs, const owned_entry& rhs) {
              return lhs.path < rhs.path;
            });

  const auto duplicate = std::adjacent_find(
      manifest_.begin(), manifest_.end(),
      [](const owned_entry& lhs, const owned_entry& rhs) {
        return lhs.path == rhs.path;
      });
  if (duplicate != manifest_.end())
  {
    throw state_error("duplicate owned path in package " + identity_.name() +
                      ": " + duplicate->path.string());
  }
}

const package_identity&
installed_package::identity() const noexcept
{
  return identity_;
}

const std::vector<owned_entry>&
installed_package::manifest() const noexcept
{
  return manifest_;
}

std::size_t
installed_package::size() const noexcept
{
  return manifest_.size();
}

const owned_entry*
installed_package::find(const pkgimage::package_path& path) const noexcept
{
  const auto found = std::lower_bound(
      manifest_.begin(), manifest_.end(), path,
      [](const owned_entry& entry, const pkgimage::package_path& searched) {
        return entry.path < searched;
      });

  if (found == manifest_.end() || found->path != path)
    return nullptr;

  return &*found;
}

bool
installed_package::owns(const pkgimage::package_path& path) const noexcept
{
  return find(path) != nullptr;
}

} // namespace pkgstate
