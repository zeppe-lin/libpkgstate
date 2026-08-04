// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file generation_codec.h
 *  \brief Canonical generation-v3 binding and snapshot records.
 */
#pragma once

#include <libpkgstate/export.h>

#include <cstdint>
#include <string_view>
#include <vector>

#include <libpkgstate/snapshot.h>
#include <libpkgstate/state_target_binding.h>

namespace pkgstate {

/*! \brief Current canonical generation record version. */
inline constexpr std::uint16_t canonical_generation_storage_version = 3;

/*! \brief Receipt-visible canonical generation protocol identifier. */
inline constexpr std::string_view canonical_generation_storage_format =
    "libpkgstate-generation-v3";

/*! \brief Encode one target binding as a canonical generation-v3 record. */
[[nodiscard]] PKGSTATE_API std::vector<std::uint8_t>
encode_generation_binding(const state_target_binding& binding);

/*! \brief Decode and canonically validate one target-binding record.
 *  \throws store_error when the record is malformed or non-canonical.
 */
[[nodiscard]] PKGSTATE_API state_target_binding
decode_generation_binding(std::string_view bytes);

/*! \brief Encode one complete installed-state snapshot canonically. */
[[nodiscard]] PKGSTATE_API std::vector<std::uint8_t>
encode_generation_snapshot(const snapshot& value);

/*! \brief Decode and canonically validate one complete snapshot record.
 *  \throws store_error when the record is malformed or non-canonical.
 */
[[nodiscard]] PKGSTATE_API snapshot
decode_generation_snapshot(std::string_view bytes);

} // namespace pkgstate
