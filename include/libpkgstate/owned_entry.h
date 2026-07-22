// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file owned_entry.h
 * \brief Canonical package-owned filesystem objects.
 */

#pragma once

#include <libpkgstate/package_path.h>

namespace pkgstate {

/*!
 * \brief Object class retained by installed ownership state.
 *
 * The legacy package database distinguishes directories by a trailing slash
 * but does not distinguish regular files, links, FIFOs, or devices from one
 * another.  The state model preserves exactly that durable distinction.
 */
enum class owned_entry_type {
  non_directory, //!< Any owned object that is not a directory.
  directory,     //!< Explicitly owned directory.
};

/*!
 * \brief One canonical path owned by an installed package.
 */
struct owned_entry final {
  package_path path; //!< Canonical root-relative installed path.
  owned_entry_type type;       //!< Durable object class.
};

/*!
 * \brief Compare owned entries for semantic equality.
 */
[[nodiscard]] bool operator==(const owned_entry& lhs,
                              const owned_entry& rhs) noexcept;

/*!
 * \brief Compare owned entries for semantic inequality.
 */
[[nodiscard]] bool operator!=(const owned_entry& lhs,
                              const owned_entry& rhs) noexcept;

} // namespace pkgstate
