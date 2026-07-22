// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/legacy_installed_package.h>

#include <algorithm>
#include <utility>

#include <libpkgstate/error.h>

namespace pkgstate {
namespace {

void
validate_entry_type(owned_entry_type type)
{
  switch (type)
  {
    case owned_entry_type::non_directory:
    case owned_entry_type::directory:
      return;
  }

  throw state_error("invalid legacy ownership entry type");
}

} // namespace

legacy_installed_package::legacy_installed_package(
    package_identity identity,
    std::vector<owned_entry> manifest)
    : identity_(std::move(identity)), manifest_(std::move(manifest))
{
  for (const owned_entry& entry : manifest_)
    validate_entry_type(entry.type);

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
    throw state_error("duplicate owned path in legacy package " +
                      identity_.name() + ": " + duplicate->path.string());
  }
}

const package_identity&
legacy_installed_package::identity() const noexcept
{
  return identity_;
}

const std::vector<owned_entry>&
legacy_installed_package::manifest() const noexcept
{
  return manifest_;
}

std::size_t
legacy_installed_package::size() const noexcept
{
  return manifest_.size();
}

const owned_entry*
legacy_installed_package::find(const package_path& path) const noexcept
{
  const auto found = std::lower_bound(
      manifest_.begin(), manifest_.end(), path,
      [](const owned_entry& entry, const package_path& searched) {
        return entry.path < searched;
      });

  if (found == manifest_.end() || found->path != path)
    return nullptr;

  return &*found;
}

bool
legacy_installed_package::owns(const package_path& path) const noexcept
{
  return find(path) != nullptr;
}

} // namespace pkgstate
