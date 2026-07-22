// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file package_identity.h
 * \brief Historical package names and opaque version lines.
 */

#pragma once

#include <string>
#include <string_view>

#include <libpkgstate/error.h>

namespace pkgstate {

/*!
 * \brief Validated identity retained by the compatibility database.
 *
 * This is not a canonical installed-package identity. The second field is an
 * opaque historical version line and is never split into canonical version and
 * distribution release coordinates. Identity fields are line-oriented
 * durable state. They must therefore be non-empty and may not contain NUL,
 * carriage-return, or newline bytes.
 * Package naming policy beyond those representation invariants belongs to a
 * higher layer.
 */
class package_identity final {
public:
  /*!
   * \brief Validate and construct a package identity.
   * \param name Package name.
   * \param version Opaque historical version line.
   * \throws identity_error when either field is empty or not line-safe.
   */
  [[nodiscard]] static package_identity
  make(std::string_view name, std::string_view version);

  /*!
   * \brief Return the package name.
   */
  [[nodiscard]] const std::string& name() const noexcept;

  /*!
   * \brief Return the opaque historical version line.
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
