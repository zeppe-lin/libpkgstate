// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../fixtures/state.h"
#include "test.h"

#include <libpkgstate/publication_codec.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <openssl/evp.h>

namespace publication_codec_fixture {

inline constexpr std::array<std::uint8_t, 8> request_magic = {
    'Z', 'L', 'S', 'P', 'R', 'Q', 'S', 'T'};
inline constexpr std::array<std::uint8_t, 8> receipt_magic = {
    'Z', 'L', 'S', 'P', 'R', 'C', 'P', 'T'};
inline constexpr std::size_t checksum_size = 32U;

inline std::array<std::uint8_t, checksum_size>
checksum(const std::vector<std::uint8_t>& bytes)
{
  using context_ptr = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
  context_ptr context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
  TEST(context != nullptr);
  TEST_EQ(EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr), 1);
  TEST_EQ(EVP_DigestUpdate(context.get(), bytes.data(), bytes.size()), 1);
  std::array<std::uint8_t, checksum_size> result{};
  unsigned int size = 0;
  TEST_EQ(EVP_DigestFinal_ex(context.get(), result.data(), &size), 1);
  TEST_EQ(size, result.size());
  return result;
}

inline void replace_checksum(std::vector<std::uint8_t>& encoding)
{
  TEST(encoding.size() >= checksum_size);
  encoding.resize(encoding.size() - checksum_size);
  const auto digest = checksum(encoding);
  encoding.insert(encoding.end(), digest.begin(), digest.end());
}

template<std::size_t Size>
bool starts_with(const std::vector<std::uint8_t>& encoding,
                 const std::array<std::uint8_t, Size>& magic)
{
  return encoding.size() >= Size &&
      std::equal(magic.begin(), magic.end(), encoding.begin());
}

template<typename Callable>
bool rejects_with(pkgstate::state_publication_codec_error_code code,
                  Callable&& callable)
{
  try
  {
    callable();
  }
  catch (const pkgstate::state_publication_codec_error& error)
  {
    return error.code() == code;
  }
  return false;
}

inline void check_request_round_trip(
    const pkgstate::state_publication_request& request,
    const pkgstate::snapshot& expected)
{
  const auto encoded = pkgstate::encode_state_publication_request(request);
  const auto decoded = pkgstate::decode_state_publication_request(encoded, expected);
  TEST_EQ(decoded.identity(), request.identity());
  TEST_EQ(decoded.expected_snapshot(), request.expected_snapshot());
  TEST_EQ(decoded.target_binding(), request.target_binding());
  TEST_EQ(decoded.deltas(), request.deltas());
  TEST_EQ(decoded.transaction_evidence(), request.transaction_evidence());
  TEST_EQ(pkgstate::encode_state_publication_request(decoded), encoded);
}

inline void check_receipt_round_trip(
    const pkgstate::state_publication_receipt& receipt,
    const pkgstate::state_publication_request& request,
    const pkgstate::snapshot& actual_prior)
{
  const auto encoded = pkgstate::encode_state_publication_receipt(receipt);
  const auto decoded = pkgstate::decode_state_publication_receipt(
      encoded, request, actual_prior);
  TEST_EQ(decoded, receipt);
  TEST_EQ(pkgstate::encode_state_publication_receipt(decoded), encoded);
}

} // namespace publication_codec_fixture
