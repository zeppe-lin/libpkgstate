// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file legacy_snapshot.h
 * \brief Immutable compatibility snapshots from `/var/lib/pkg/db`.
 */

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <libpkgstate/digest.h>
#include <libpkgstate/legacy_completeness.h>
#include <libpkgstate/legacy_installed_package.h>

namespace pkgstate {

/*!
 * \brief Indexed view of one historical package-database observation.
 *
 * This value is explicitly incomplete installed truth.  It has no target
 * binding, canonical package releases, installed control, typed installed
 * identities, or canonical snapshot identity.  Its separate observation
 * identity commits only to normalized compatibility records.
 */
class legacy_snapshot final {
public:
  /*!
   * \brief Construct and index compatibility package records.
   * \throws state_error when a package name appears more than once.
   */
  explicit legacy_snapshot(
      std::vector<legacy_installed_package> packages = {});

  /*! \brief Return the exact identity of this incomplete observation. */
  [[nodiscard]] const legacy_snapshot_observation_identity&
  observation_identity() const noexcept;

  /*! \brief Return the fixed historical completeness profile. */
  [[nodiscard]] legacy_snapshot_completeness completeness() const noexcept;

  /*! \brief Return records in package-name order. */
  [[nodiscard]] const std::vector<legacy_installed_package>&
  packages() const noexcept;

  /*! \brief Return the number of compatibility package records. */
  [[nodiscard]] std::size_t size() const noexcept;

  /*! \brief Find a compatibility package record by name. */
  [[nodiscard]] const legacy_installed_package*
  find_package(std::string_view name) const noexcept;

  /*! \brief Return every compatibility record that owns an exact path. */
  [[nodiscard]] std::vector<const legacy_installed_package*>
  owners(const package_path& path) const;

  /*! \brief Test whether any compatibility record owns an exact path. */
  [[nodiscard]] bool
  is_owned(const package_path& path) const noexcept;

private:
  std::vector<legacy_installed_package> packages_;
  legacy_snapshot_observation_identity observation_identity_;
  std::unordered_map<std::string, std::vector<std::size_t>> owners_;
};

} // namespace pkgstate
