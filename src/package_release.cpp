// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/package_release.h>

#include "canonical_record.h"

#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include <libpkgstate/error.h>

namespace pkgstate {
namespace {

void
validate_coordinate(std::string_view value, const char* label)
{
  if (value.empty())
    throw identity_error(std::string(label) + " must not be empty");

  for (const char byte : value)
  {
    if (byte == '\0' || byte == '\n' || byte == '\r')
      throw identity_error(std::string(label) + " is not line-safe");
  }
}

package_release_identity
identify_release(std::string_view name,
                 std::string_view version,
                 std::string_view release)
{
  detail::canonical_record record(package_release_identity::canonical_domain());
  record.append_bytes(name);
  record.append_bytes(version);
  record.append_bytes(release);
  return package_release_identity::from_sha256(record.sha256());
}

} // namespace

package_release
package_release::make(std::string_view name,
                      std::string_view version,
                      std::string_view release)
{
  validate_coordinate(name, "package name");
  validate_coordinate(version, "package version");
  validate_coordinate(release, "package release");

  return package_release(identify_release(name, version, release),
                         std::string(name),
                         std::string(version),
                         std::string(release));
}

package_release::package_release(package_release_identity identity,
                                 std::string name,
                                 std::string version,
                                 std::string release)
    : identity_(std::move(identity)),
      name_(std::move(name)),
      version_(std::move(version)),
      release_(std::move(release))
{
}

const package_release_identity&
package_release::identity() const noexcept
{
  return identity_;
}

const std::string&
package_release::name() const noexcept
{
  return name_;
}

const std::string&
package_release::version() const noexcept
{
  return version_;
}

const std::string&
package_release::release() const noexcept
{
  return release_;
}

bool
operator==(const package_release& lhs, const package_release& rhs) noexcept
{
  return lhs.identity_ == rhs.identity_ && lhs.name_ == rhs.name_ &&
         lhs.version_ == rhs.version_ && lhs.release_ == rhs.release_;
}

bool
operator!=(const package_release& lhs, const package_release& rhs) noexcept
{
  return !(lhs == rhs);
}

bool
operator<(const package_release& lhs, const package_release& rhs) noexcept
{
  return std::tie(lhs.name_, lhs.version_, lhs.release_, lhs.identity_) <
         std::tie(rhs.name_, rhs.version_, rhs.release_, rhs.identity_);
}

} // namespace pkgstate
