/*
 * Copyright (C) 2026 Alexandr Savca
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*!
 * \file snapshot.h
 * \brief Immutable installed-state snapshots and ownership queries.
 * \copyright See COPYING for license terms and COPYRIGHT for notices.
 */

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <libpkgstate/installed_package.h>

namespace pkgstate {

/*!
 * \brief Immutable, indexed view of installed package state.
 *
 * Packages are sorted by name.  Duplicate package names are rejected.
 * Multiple packages may own the same path; owners() exposes that state
 * explicitly in package-name order.
 */
class snapshot final {
public:
  /*!
   * \brief Construct and index an installed-state snapshot.
   * \param packages Installed packages in arbitrary order.
   * \throws state_error when a package name appears more than once.
   */
  explicit snapshot(std::vector<installed_package> packages = {});

  /*!
   * \brief Return all packages in package-name order.
   */
  [[nodiscard]] const std::vector<installed_package>&
  packages() const noexcept;

  /*!
   * \brief Return the number of installed package records.
   */
  [[nodiscard]] std::size_t size() const noexcept;

  /*!
   * \brief Find an installed package by name.
   * \return Pointer to the package, or nullptr when absent.
   */
  [[nodiscard]] const installed_package*
  find_package(std::string_view name) const noexcept;

  /*!
   * \brief Return every package that owns a canonical path.
   * \return Owners in package-name order.  The vector is empty when the path
   *         is unowned.
   */
  [[nodiscard]] std::vector<const installed_package*>
  owners(const pkgimage::package_path& path) const;

  /*!
   * \brief Test whether any package owns a path.
   */
  [[nodiscard]] bool
  is_owned(const pkgimage::package_path& path) const noexcept;

private:
  std::vector<installed_package> packages_;
  std::unordered_map<std::string, std::vector<std::size_t>> owners_;
};

} // namespace pkgstate
