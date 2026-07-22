// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file package_release.h
 * \brief Canonical package-release facts.
 */

#pragma once

#include <string>
#include <string_view>

#include <libpkgstate/digest.h>

namespace pkgstate {

/*!
 * \brief One canonical package release and its descriptive coordinates.
 *
 * The identity is computed by libpkgstate from the canonical name, version,
 * and distribution release.  Relational ordering is deterministic record
 * order, not package-version precedence.
 */
class package_release final {
public:
  /*!
   * \brief Validate and construct one canonical package release.
   * \throws identity_error when a coordinate is empty or not line-safe.
   */
  [[nodiscard]] static package_release
  make(std::string_view name,
       std::string_view version,
       std::string_view release);

  /*! \brief Return the canonical package-release identity. */
  [[nodiscard]] const package_release_identity& identity() const noexcept;

  /*! \brief Return the package name. */
  [[nodiscard]] const std::string& name() const noexcept;

  /*! \brief Return the upstream package version. */
  [[nodiscard]] const std::string& version() const noexcept;

  /*! \brief Return the distribution package release. */
  [[nodiscard]] const std::string& release() const noexcept;

  friend bool operator==(const package_release& lhs,
                         const package_release& rhs) noexcept;
  friend bool operator!=(const package_release& lhs,
                         const package_release& rhs) noexcept;
  friend bool operator<(const package_release& lhs,
                        const package_release& rhs) noexcept;

private:
  package_release(package_release_identity identity,
                  std::string name,
                  std::string version,
                  std::string release);

  package_release_identity identity_;
  std::string name_;
  std::string version_;
  std::string release_;
};

} // namespace pkgstate
