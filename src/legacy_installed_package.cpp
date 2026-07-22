// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/legacy_installed_package.h>

#include <algorithm>
#include <cstdint>
#include <utility>

#include <libpkgstate/error.h>

#include "canonical_record.h"

namespace pkgstate {
namespace {

std::uint8_t
canonical_entry_type(owned_entry_type type)
{
  switch (type)
  {
    case owned_entry_type::non_directory:
      return 1;
    case owned_entry_type::directory:
      return 2;
  }

  throw state_error("invalid legacy ownership entry type");
}

std::vector<owned_entry>
normalize_manifest(const package_identity& identity,
                   std::vector<owned_entry> manifest)
{
  for (const owned_entry& entry : manifest)
    static_cast<void>(canonical_entry_type(entry.type));

  std::sort(manifest.begin(), manifest.end(),
            [](const owned_entry& lhs, const owned_entry& rhs) {
              return lhs.path < rhs.path;
            });

  const auto duplicate = std::adjacent_find(
      manifest.begin(), manifest.end(),
      [](const owned_entry& lhs, const owned_entry& rhs) {
        return lhs.path == rhs.path;
      });
  if (duplicate != manifest.end())
  {
    throw state_error("duplicate owned path in legacy package " +
                      identity.name() + ": " + duplicate->path.string());
  }

  return manifest;
}

legacy_package_observation_identity
compute_observation_identity(const package_identity& identity,
                             const std::vector<owned_entry>& manifest)
{
  detail::canonical_record record(
      legacy_package_observation_identity::canonical_domain());
  record.append_bytes(identity.name());
  record.append_bytes(identity.version());
  record.append_u64(static_cast<std::uint64_t>(manifest.size()));
  for (const owned_entry& entry : manifest)
  {
    record.append_bytes(entry.path.string());
    record.append_u8(canonical_entry_type(entry.type));
  }

  return legacy_package_observation_identity::from_sha256(record.sha256());
}

} // namespace

legacy_installed_package::legacy_installed_package(
    package_identity identity,
    std::vector<owned_entry> manifest)
    : identity_(std::move(identity)),
      manifest_(normalize_manifest(identity_, std::move(manifest))),
      observation_identity_(compute_observation_identity(identity_, manifest_))
{
}

const package_identity&
legacy_installed_package::identity() const noexcept
{
  return identity_;
}

const legacy_package_observation_identity&
legacy_installed_package::observation_identity() const noexcept
{
  return observation_identity_;
}

legacy_package_completeness
legacy_installed_package::completeness() const noexcept
{
  return {};
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
