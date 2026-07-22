// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file legacy_installed_package.h
 * \brief Incomplete installed records from the historical package database.
 */

#pragma once

#include <cstddef>
#include <vector>

#include <libpkgstate/owned_entry.h>
#include <libpkgstate/package_identity.h>

namespace pkgstate {

/*!
 * \brief Compatibility record decoded from `/var/lib/pkg/db`.
 *
 * The historical format retains a package name, one opaque version line, and
 * an ownership manifest.  It does not retain a canonical package release,
 * installed control, target binding, application evidence, or canonical
 * installed-package identity.  This type preserves that limitation instead of
 * inventing missing canonical facts.
 */
class legacy_installed_package final {
public:
  /*!
   * \brief Construct and validate one compatibility package record.
   * \param identity Historical package name and opaque version line.
   * \param manifest Canonical paths retained by the compatibility database.
   * \throws state_error when the manifest contains duplicate paths or an
   *         invalid object class.
   */
  legacy_installed_package(package_identity identity,
                           std::vector<owned_entry> manifest);

  /*! \brief Return the historical package identity. */
  [[nodiscard]] const package_identity& identity() const noexcept;

  /*! \brief Return the canonical path-sorted ownership manifest. */
  [[nodiscard]] const std::vector<owned_entry>& manifest() const noexcept;

  /*! \brief Return the number of retained ownership entries. */
  [[nodiscard]] std::size_t size() const noexcept;

  /*! \brief Find one retained ownership entry by canonical path. */
  [[nodiscard]] const owned_entry*
  find(const package_path& path) const noexcept;

  /*! \brief Test whether the compatibility record owns a path. */
  [[nodiscard]] bool owns(const package_path& path) const noexcept;

private:
  package_identity identity_;
  std::vector<owned_entry> manifest_;
};

} // namespace pkgstate
