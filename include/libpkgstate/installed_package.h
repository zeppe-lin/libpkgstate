/*
 * Copyright (C) 2026 Alexandr Savca
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*!
 * \file installed_package.h
 * \brief Durable installed package ownership state.
 * \copyright See COPYING for license terms and COPYRIGHT for notices.
 */

#pragma once

#include <cstddef>
#include <vector>

#include <libpkgstate/error.h>
#include <libpkgstate/owned_entry.h>
#include <libpkgstate/package_identity.h>

namespace pkgstate {

/*!
 * \brief Installed package identity and canonical ownership manifest.
 *
 * An installed manifest describes durable ownership after policy resolution.
 * It is not the raw archive manifest.  Entries are sorted by path and
 * duplicate canonical paths are rejected.  Empty manifests are representable.
 */
class installed_package final {
public:
  /*!
   * \brief Construct and validate installed package state.
   * \param identity Package name and version.
   * \param manifest Canonical objects owned by the installed package.
   * \throws state_error when the manifest contains duplicate paths.
   */
  installed_package(package_identity identity,
                    std::vector<owned_entry> manifest);

  /*!
   * \brief Return the package identity.
   */
  [[nodiscard]] const package_identity& identity() const noexcept;

  /*!
   * \brief Return the canonical path-sorted ownership manifest.
   */
  [[nodiscard]] const std::vector<owned_entry>& manifest() const noexcept;

  /*!
   * \brief Return the number of owned objects.
   */
  [[nodiscard]] std::size_t size() const noexcept;

  /*!
   * \brief Find an owned object by canonical path.
   * \return Pointer to the entry, or nullptr when the package does not own
   *         the path.
   */
  [[nodiscard]] const owned_entry*
  find(const pkgimage::package_path& path) const noexcept;

  /*!
   * \brief Test whether the package owns a path.
   */
  [[nodiscard]] bool owns(const pkgimage::package_path& path) const noexcept;

private:
  package_identity identity_;
  std::vector<owned_entry> manifest_;
};

} // namespace pkgstate
