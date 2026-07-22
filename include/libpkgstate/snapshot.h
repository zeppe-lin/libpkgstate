// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file snapshot.h
 * \brief Complete immutable installed-state snapshots.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <libpkgstate/installed_package.h>
#include <libpkgstate/state_target_binding.h>

namespace pkgstate {

/*! \brief Current canonical installed-state snapshot schema. */
inline constexpr std::uint16_t installed_state_schema_version = 1;

/*!
 * \brief Complete immutable installed truth for one target-state binding.
 *
 * Every contained package embeds its canonical release, installed control,
 * target binding, and ownership manifest.  Construction rejects packages from
 * another binding and duplicate package names.  Shared path ownership remains
 * valid and is exposed in deterministic package-name order.
 *
 * Snapshot and ownership-inventory identities are added by the next model
 * layer.  This value already contains the complete facts those identities
 * cover.
 */
class snapshot final {
public:
  /*!
   * \brief Validate and construct one complete installed-state snapshot.
   * \param target_binding Durable state target represented by the snapshot.
   * \param packages Complete installed package records in arbitrary order.
   * \throws state_error on duplicate names or target-binding mismatch.
   */
  [[nodiscard]] static snapshot
  make(state_target_binding target_binding,
       std::vector<installed_package> packages = {});

  /*! \brief Return the canonical snapshot schema version. */
  [[nodiscard]] std::uint16_t schema_version() const noexcept;

  /*! \brief Return the target-state binding shared by every package. */
  [[nodiscard]] const state_target_binding&
  target_binding() const noexcept;

  /*! \brief Return all packages in package-name order. */
  [[nodiscard]] const std::vector<installed_package>&
  packages() const noexcept;

  /*! \brief Return the number of installed package records. */
  [[nodiscard]] std::size_t size() const noexcept;

  /*! \brief Find an installed package by canonical package name. */
  [[nodiscard]] const installed_package*
  find_package(std::string_view name) const noexcept;

  /*! \brief Return every package that owns one canonical path. */
  [[nodiscard]] std::vector<const installed_package*>
  owners(const package_path& path) const;

  /*! \brief Test whether any package owns one canonical path. */
  [[nodiscard]] bool
  is_owned(const package_path& path) const noexcept;

private:
  snapshot(state_target_binding target_binding,
           std::vector<installed_package> packages,
           std::unordered_map<std::string, std::vector<std::size_t>> owners);

  state_target_binding target_binding_;
  std::vector<installed_package> packages_;
  std::unordered_map<std::string, std::vector<std::size_t>> owners_;
};

} // namespace pkgstate
