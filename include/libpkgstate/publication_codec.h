// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file publication_codec.h
 * \brief Canonical durable encodings for state-publication evidence.
 */

#pragma once

#include <libpkgstate/export.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <libpkgstate/publication_receipt.h>
#include <libpkgstate/publication_request.h>
#include <libpkgstate/snapshot.h>

namespace pkgstate {

/*! \brief Current state-publication request encoding version. */
inline constexpr std::uint16_t state_publication_request_encoding_version = 2;

/*! \brief Current state-publication receipt encoding version. */
inline constexpr std::uint16_t state_publication_receipt_encoding_version = 2;

/*! \brief Maximum admitted request record size. */
inline constexpr std::size_t maximum_state_publication_request_encoding_size =
    1024ULL * 1024ULL * 1024ULL;

/*! \brief Maximum admitted receipt record size. */
inline constexpr std::size_t maximum_state_publication_receipt_encoding_size =
    64ULL * 1024ULL * 1024ULL;

/*! \brief Typed publication-codec failure class. */
enum class state_publication_codec_error_code : std::uint8_t {
  limit_exceeded = 1,
  invalid_magic = 2,
  unsupported_version = 3,
  truncated = 4,
  trailing_data = 5,
  checksum_mismatch = 6,
  invalid_value = 7,
  expected_snapshot_mismatch = 8,
  request_mismatch = 9,
  actual_prior_mismatch = 10,
  identity_mismatch = 11,
};

/*! \brief Reports malformed publication evidence or foreign decode authority. */
class PKGSTATE_API state_publication_codec_error final : public std::invalid_argument {
public:
  state_publication_codec_error(state_publication_codec_error_code code,
                                std::string message);
  ~state_publication_codec_error() override;

  [[nodiscard]] state_publication_codec_error_code code() const noexcept;

private:
  state_publication_codec_error_code code_;
};

using state_publication_request_encoding = std::vector<std::uint8_t>;
using state_publication_receipt_encoding = std::vector<std::uint8_t>;

/*! \brief Canonically encode one immutable publication request. */
[[nodiscard]] PKGSTATE_API state_publication_request_encoding
encode_state_publication_request(const state_publication_request& request);

/*!
 * \brief Decode one request under its exact expected-snapshot authority.
 *
 * The expected snapshot is not reconstructed from the record. Proposed
 * installed packages are retained as complete native state bodies and every
 * delta is rebuilt through its public invariant-enforcing factory.
 */
[[nodiscard]] PKGSTATE_API state_publication_request
decode_state_publication_request(
    const state_publication_request_encoding& encoding,
    const snapshot& expected_snapshot);

/*! \brief Canonically encode one immutable publication receipt. */
[[nodiscard]] PKGSTATE_API state_publication_receipt_encoding
encode_state_publication_receipt(const state_publication_receipt& receipt);

/*!
 * \brief Decode one receipt under exact request and actual-prior authority.
 *
 * Any resulting snapshot is derived from the request and actual prior state;
 * it is not accepted as a caller-authored replacement snapshot.
 */
[[nodiscard]] PKGSTATE_API state_publication_receipt
decode_state_publication_receipt(
    const state_publication_receipt_encoding& encoding,
    const state_publication_request& request,
    const snapshot& actual_prior);

} // namespace pkgstate
