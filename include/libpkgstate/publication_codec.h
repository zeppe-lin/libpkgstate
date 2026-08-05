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
inline constexpr std::uint16_t state_publication_request_encoding_version = 1;

/*! \brief Current state-publication receipt encoding version. */
inline constexpr std::uint16_t state_publication_receipt_encoding_version = 1;

/*! \brief Maximum admitted request record size. */
inline constexpr std::size_t maximum_state_publication_request_encoding_size =
    1024ULL * 1024ULL * 1024ULL;

/*! \brief Maximum admitted receipt record size. */
inline constexpr std::size_t maximum_state_publication_receipt_encoding_size =
    64ULL * 1024ULL * 1024ULL;

/*! \brief Typed publication-codec failure class. */
enum class state_publication_codec_error_code : std::uint8_t {
  limit_exceeded = 1, ///< Input exceeds the documented byte ceiling.
  invalid_magic = 2, ///< The record does not carry the publication magic.
  unsupported_version = 3, ///< The record uses an unsupported schema version.
  truncated = 4, ///< The record ends before a complete value is available.
  trailing_data = 5, ///< Bytes remain after the complete canonical record.
  checksum_mismatch = 6, ///< The record checksum does not match its payload.
  invalid_value = 7, ///< A decoded field violates its owner value contract.
  expected_snapshot_mismatch = 8, ///< Expected-state authority is inconsistent.
  request_mismatch = 9, ///< Receipt material does not match its request.
  actual_prior_mismatch = 10, ///< Prior-state evidence is internally inconsistent.
  identity_mismatch = 11, ///< A recomputed semantic identity does not match.
};

/*! \brief Reports malformed publication evidence or foreign decode authority. */
class PKGSTATE_API state_publication_codec_error final : public std::invalid_argument {
public:
  /*!
   * \brief Construct a typed publication-codec failure.
   * \param code Stable codec failure category.
   * \param message Human-readable diagnostic message.
   */
  state_publication_codec_error(state_publication_codec_error_code code,
                                std::string message);

  /*! \brief Destroy the polymorphic publication-codec failure. */
  ~state_publication_codec_error() override;

  /*!
   * \brief Return the stable codec failure category.
   * \return Category supplied at construction.
   */
  [[nodiscard]] state_publication_codec_error_code code() const noexcept;

private:
  state_publication_codec_error_code code_;
};

/*! \brief Canonical durable bytes for one publication request. */
using state_publication_request_encoding = std::vector<std::uint8_t>;

/*! \brief Canonical durable bytes for one publication receipt. */
using state_publication_receipt_encoding = std::vector<std::uint8_t>;

/*!
 * \brief Canonically encode one immutable publication request.
 * \param request Exact publication request.
 * \return Canonical publication-request bytes.
 */
[[nodiscard]] PKGSTATE_API state_publication_request_encoding
encode_state_publication_request(const state_publication_request& request);

/*!
 * \brief Decode one request under its exact expected-snapshot authority.
 *
 * The expected snapshot is not reconstructed from the record. Proposed
 * installed packages are retained as complete native state bodies and every
 * delta is rebuilt through its public invariant-enforcing factory.
 * \param encoding Canonical encoded record to decode.
 * \param expected_snapshot Exact expected-snapshot authority supplied by the
 *                          caller.
 * \return Decoded publication request bound to the supplied expected snapshot.
 */
[[nodiscard]] PKGSTATE_API state_publication_request
decode_state_publication_request(
    const state_publication_request_encoding& encoding,
    const snapshot& expected_snapshot);

/*!
 * \brief Canonically encode one immutable publication receipt.
 * \param receipt Publication receipt to encode.
 * \return Canonical publication-receipt bytes.
 */
[[nodiscard]] PKGSTATE_API state_publication_receipt_encoding
encode_state_publication_receipt(const state_publication_receipt& receipt);

/*!
 * \brief Decode one receipt under exact request and actual-prior authority.
 *
 * Any resulting snapshot is derived from the request and actual prior state;
 * it is not accepted as a caller-authored replacement snapshot.
 * \param encoding Canonical encoded record to decode.
 * \param request Exact publication request.
 * \param actual_prior Authoritative prior snapshot observed by the state
 *                     store.
 * \return Decoded publication receipt bound to the supplied request and prior
 *         state.
 */
[[nodiscard]] PKGSTATE_API state_publication_receipt
decode_state_publication_receipt(
    const state_publication_receipt_encoding& encoding,
    const state_publication_request& request,
    const snapshot& actual_prior);

} // namespace pkgstate
