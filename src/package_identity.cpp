/*
 * Copyright (C) 2026 Alexandr Savca
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <libpkgstate/package_identity.h>

#include <libpkgstate/error.h>

namespace pkgstate {
namespace {

void
validate_field(std::string_view field, const char* label)
{
  if (field.empty())
    throw identity_error(std::string(label) + " must not be empty");

  for (const char byte : field)
  {
    if (byte == '\0' || byte == '\n' || byte == '\r')
      throw identity_error(std::string(label) + " is not line-safe");
  }
}

} // namespace

package_identity
package_identity::make(std::string_view name, std::string_view version)
{
  validate_field(name, "package name");
  validate_field(version, "package version");
  return package_identity(std::string(name), std::string(version));
}

package_identity::package_identity(std::string name, std::string version)
    : name_(std::move(name)), version_(std::move(version))
{
}

const std::string&
package_identity::name() const noexcept
{
  return name_;
}

const std::string&
package_identity::version() const noexcept
{
  return version_;
}

bool
operator==(const package_identity& lhs,
           const package_identity& rhs) noexcept
{
  return lhs.name_ == rhs.name_ && lhs.version_ == rhs.version_;
}

bool
operator!=(const package_identity& lhs,
           const package_identity& rhs) noexcept
{
  return !(lhs == rhs);
}

} // namespace pkgstate
