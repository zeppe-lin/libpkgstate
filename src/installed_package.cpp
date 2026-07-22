// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/installed_package.h>

#include "canonical_record.h"

#include <algorithm>
#include <cstdint>
#include <tuple>
#include <utility>

#include <libpkgstate/error.h>

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

  throw state_error("invalid installed ownership entry type");
}

void
normalize_manifest(std::vector<owned_entry>& manifest,
                   const package_release& release)
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
    throw state_error("duplicate owned path in installed package " +
                      release.name() + ": " + duplicate->path.string());
  }
}

installed_package_identity
identify_package(const package_release& release,
                 const installed_control& control,
                 const state_target_binding& target_binding,
                 const std::vector<owned_entry>& manifest)
{
  detail::canonical_record record(
      installed_package_identity::canonical_domain());
  record.append_digest(release.identity());
  record.append_digest(control.identity());
  record.append_digest(target_binding.identity());
  record.append_u64(static_cast<std::uint64_t>(manifest.size()));

  for (const owned_entry& entry : manifest)
  {
    record.append_bytes(entry.path.string());
    record.append_u8(canonical_entry_type(entry.type));
  }

  return installed_package_identity::from_sha256(record.sha256());
}

} // namespace

installed_package
installed_package::make(package_release release,
                        installed_control control,
                        state_target_binding target_binding,
                        std::vector<owned_entry> manifest)
{
  if (control.release() != release)
  {
    throw state_error("installed control release does not match package " +
                      release.name());
  }

  normalize_manifest(manifest, release);
  installed_package_identity identity = identify_package(
      release, control, target_binding, manifest);

  return installed_package(std::move(identity),
                           std::move(release),
                           std::move(control),
                           std::move(target_binding),
                           std::move(manifest));
}

installed_package::installed_package(
    installed_package_identity identity,
    package_release release,
    installed_control control,
    state_target_binding target_binding,
    std::vector<owned_entry> manifest)
    : identity_(std::move(identity)),
      release_(std::move(release)),
      control_(std::move(control)),
      target_binding_(std::move(target_binding)),
      manifest_(std::move(manifest))
{
}

const installed_package_identity&
installed_package::identity() const noexcept
{
  return identity_;
}

const package_release&
installed_package::release() const noexcept
{
  return release_;
}

const installed_control&
installed_package::control() const noexcept
{
  return control_;
}

const state_target_binding&
installed_package::target_binding() const noexcept
{
  return target_binding_;
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
installed_package::find(const package_path& path) const noexcept
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
installed_package::owns(const package_path& path) const noexcept
{
  return find(path) != nullptr;
}

bool
operator==(const installed_package& lhs,
           const installed_package& rhs) noexcept
{
  return lhs.identity_ == rhs.identity_ &&
         lhs.release_ == rhs.release_ &&
         lhs.control_ == rhs.control_ &&
         lhs.target_binding_ == rhs.target_binding_ &&
         lhs.manifest_ == rhs.manifest_;
}

bool
operator!=(const installed_package& lhs,
           const installed_package& rhs) noexcept
{
  return !(lhs == rhs);
}

bool
operator<(const installed_package& lhs,
          const installed_package& rhs) noexcept
{
  return std::tie(lhs.release_,
                  lhs.target_binding_,
                  lhs.control_,
                  lhs.identity_) <
         std::tie(rhs.release_,
                  rhs.target_binding_,
                  rhs.control_,
                  rhs.identity_);
}

} // namespace pkgstate
