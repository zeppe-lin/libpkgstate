// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/package_release.h>

#include <tuple>
#include <utility>

#include <libpkgstate/error.h>

namespace pkgstate {
namespace {

bool valid_version(const std::string& value)
{
  if (value.empty() || value.find('/') != std::string::npos)
    return false;
  for (const unsigned char byte : value)
    if (byte == 0 || byte == '\n' || byte == '\r' || byte < 0x20 || byte == 0x7f)
      return false;
  return true;
}

} // namespace

package_release::package_release(package_release_identity identity,
                                 package_reference package,
                                 std::string version,
                                 std::uint32_t release)
    : identity_(std::move(identity)), package_(std::move(package)),
      version_(std::move(version)), release_(release)
{
  if (!valid_version(version_) || release_ == 0)
    throw state_error("invalid package version or release");
}

const package_release_identity& package_release::identity() const noexcept { return identity_; }
const package_reference& package_release::package() const noexcept { return package_; }
const std::string& package_release::name() const noexcept { return package_.name(); }
const std::string& package_release::version() const noexcept { return version_; }
std::uint32_t package_release::release() const noexcept { return release_; }
std::string package_release::version_release() const { return version_ + "-" + std::to_string(release_); }
bool operator==(const package_release& lhs, const package_release& rhs) noexcept { return std::tie(lhs.identity_, lhs.package_, lhs.version_, lhs.release_) == std::tie(rhs.identity_, rhs.package_, rhs.version_, rhs.release_); }
bool operator!=(const package_release& lhs, const package_release& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const package_release& lhs, const package_release& rhs) noexcept { return std::tie(lhs.package_, lhs.version_, lhs.release_, lhs.identity_) < std::tie(rhs.package_, rhs.version_, rhs.release_, rhs.identity_); }

} // namespace pkgstate
