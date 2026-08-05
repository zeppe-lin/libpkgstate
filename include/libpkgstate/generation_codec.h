// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file generation_codec.h
 *  \brief Canonical generation-v1 binding and snapshot records.
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
inline constexpr std::uint16_t canonical_generation_storage_version = 1;

/*! \brief Receipt-visible canonical generation protocol identifier. */
inline constexpr std::string_view canonical_generation_storage_format =
    "libpkgstate-generation-v1";

/*!
 * \brief Encode one target binding as a canonical generation-v1 record.
 * \param binding Target and state-store binding to encode.
 * \return Canonical generation-v1 target-binding bytes.
 */
[[nodiscard]] PKGSTATE_API std::vector<std::uint8_t>
encode_generation_binding(const state_target_binding& binding);

/*!
 * \brief Decode and canonically validate one target-binding record.
 *  \throws store_error when the record is malformed or non-canonical.
 * \param bytes Canonical record bytes to decode.
 * \return Decoded canonical target binding.
 */
[[nodiscard]] PKGSTATE_API state_target_binding
decode_generation_binding(std::string_view bytes);

/*!
 * \brief Encode one complete installed-state snapshot canonically.
 * \param value Complete installed-state snapshot to encode.
 * \return Canonical generation-v1 snapshot bytes.
 */
[[nodiscard]] PKGSTATE_API std::vector<std::uint8_t>
encode_generation_snapshot(const snapshot& value);

/*!
 * \brief Decode and canonically validate one complete snapshot record.
 *  \throws store_error when the record is malformed or non-canonical.
 * \param bytes Canonical record bytes to decode.
 * \return Decoded canonical installed-state snapshot.
 */
[[nodiscard]] PKGSTATE_API snapshot
decode_generation_snapshot(std::string_view bytes);

} // namespace pkgstate
