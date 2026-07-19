// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file package_identity.h
 * \brief Installed package names and versions.
 */

#pragma once

#include <string>
#include <string_view>

#include <libpkgstate/error.h>

namespace pkgstate {

/*!
 * \brief Validated identity of one installed package.
 *
 * Identity fields are line-oriented durable state.  They must therefore be
 * non-empty and may not contain NUL, carriage-return, or newline bytes.
 * Package naming policy beyond those representation invariants belongs to a
 * higher layer.
 */
class package_identity final {
public:
  /*!
   * \brief Validate and construct a package identity.
   * \param name Package name.
   * \param version Package version string.
   * \throws identity_error when either field is empty or not line-safe.
   */
  [[nodiscard]] static package_identity
  make(std::string_view name, std::string_view version);

  /*!
   * \brief Return the package name.
   */
  [[nodiscard]] const std::string& name() const noexcept;

  /*!
   * \brief Return the package version.
   */
  [[nodiscard]] const std::string& version() const noexcept;

  /*!
   * \brief Compare package identities for equality.
   */
  friend bool operator==(const package_identity& lhs,
                         const package_identity& rhs) noexcept;

  /*!
   * \brief Compare package identities for inequality.
   */
  friend bool operator!=(const package_identity& lhs,
                         const package_identity& rhs) noexcept;

private:
  package_identity(std::string name, std::string version);

  std::string name_;
  std::string version_;
};

} // namespace pkgstate
