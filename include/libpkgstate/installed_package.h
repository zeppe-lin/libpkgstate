// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file installed_package.h
 * \brief Complete canonical installed package records.
 */

#pragma once

#include <cstddef>
#include <vector>

#include <libpkgstate/digest.h>
#include <libpkgstate/installed_control.h>
#include <libpkgstate/owned_entry.h>
#include <libpkgstate/package_release.h>
#include <libpkgstate/state_target_binding.h>

namespace pkgstate {

/*!
 * \brief Complete canonical installed truth for one package release.
 *
 * The record binds one canonical package release, its durable historical
 * control, the target-state domain in which it is installed, and the completed
 * ownership manifest.  libpkgstate computes the installed-package identity
 * from those facts.  The record contains no installed-snapshot identity, so
 * the containing snapshot may identify itself without an identity cycle.
 */
class installed_package final {
public:
  /*!
   * \brief Validate, normalize, identify, and construct installed truth.
   * \param release Canonical installed package release.
   * \param control Durable control for exactly the same release.
   * \param target_binding Durable target-state domain.
   * \param manifest Completed installed ownership manifest.
   * \throws state_error on release mismatch, duplicate paths, invalid object
   *         classes, or canonical identity failure.
   */
  [[nodiscard]] static installed_package
  make(package_release release,
       installed_control control,
       state_target_binding target_binding,
       std::vector<owned_entry> manifest);

  /*! \brief Return the computed installed-package identity. */
  [[nodiscard]] const installed_package_identity& identity() const noexcept;

  /*! \brief Return the exact installed package release. */
  [[nodiscard]] const package_release& release() const noexcept;

  /*! \brief Return the durable historical installed control. */
  [[nodiscard]] const installed_control& control() const noexcept;

  /*! \brief Return the durable target-state binding. */
  [[nodiscard]] const state_target_binding&
  target_binding() const noexcept;

  /*! \brief Return the canonical path-sorted ownership manifest. */
  [[nodiscard]] const std::vector<owned_entry>& manifest() const noexcept;

  /*! \brief Return the number of owned objects. */
  [[nodiscard]] std::size_t size() const noexcept;

  /*! \brief Find an owned object by canonical path. */
  [[nodiscard]] const owned_entry*
  find(const package_path& path) const noexcept;

  /*! \brief Test whether the package owns a path. */
  [[nodiscard]] bool owns(const package_path& path) const noexcept;

  friend bool operator==(const installed_package& lhs,
                         const installed_package& rhs) noexcept;
  friend bool operator!=(const installed_package& lhs,
                         const installed_package& rhs) noexcept;
  friend bool operator<(const installed_package& lhs,
                        const installed_package& rhs) noexcept;

private:
  installed_package(installed_package_identity identity,
                    package_release release,
                    installed_control control,
                    state_target_binding target_binding,
                    std::vector<owned_entry> manifest);

  installed_package_identity identity_;
  package_release release_;
  installed_control control_;
  state_target_binding target_binding_;
  std::vector<owned_entry> manifest_;
};

} // namespace pkgstate
