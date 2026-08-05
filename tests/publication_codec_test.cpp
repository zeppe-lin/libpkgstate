// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "native_fixture.h"
#include "test.h"

#include <libpkgstate/publication_codec.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <openssl/evp.h>

namespace {

using namespace pkgstate;

constexpr std::array<std::uint8_t, 8> request_magic = {
    'Z', 'L', 'S', 'P', 'R', 'Q', 'S', 'T',
};
constexpr std::array<std::uint8_t, 8> receipt_magic = {
    'Z', 'L', 'S', 'P', 'R', 'C', 'P', 'T',
};
constexpr std::size_t checksum_size = 32U;

std::array<std::uint8_t, checksum_size> checksum(
    const std::vector<std::uint8_t>& bytes)
{
  using context_ptr =
      std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
  context_ptr context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
  TEST(context != nullptr);
  TEST_EQ(EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr), 1);
  TEST_EQ(EVP_DigestUpdate(
      context.get(), bytes.data(), bytes.size()), 1);

  std::array<std::uint8_t, checksum_size> result{};
  unsigned int size = 0;
  TEST_EQ(EVP_DigestFinal_ex(context.get(), result.data(), &size), 1);
  TEST_EQ(size, result.size());
  return result;
}

void replace_checksum(std::vector<std::uint8_t>& encoding)
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

package_state_delta install_delta(installed_package package)
{
  operation_plan_identity plan = package.receipt().operation_plan();
  application_evidence_identity evidence =
      package.receipt().application_evidence();
  return package_state_delta::install(
      std::move(package), std::move(plan), std::move(evidence));
}

state_publication_request install_request(
    const snapshot& prior,
    std::string name = "example",
    std::uint8_t seed = 20)
{
  return state_publication_request::make(
      prior,
      {install_delta(native_fixture::package(
          std::move(name), seed, prior.target_binding()))});
}

snapshot projected_install(const state_publication_request& request)
{
  return snapshot::make(
      request.target_binding(),
      {*request.deltas().front().proposed_package()});
}

void check_request_round_trip(const state_publication_request& request,
                              const snapshot& expected)
{
  const state_publication_request_encoding encoded =
      encode_state_publication_request(request);
  const state_publication_request decoded =
      decode_state_publication_request(encoded, expected);
  TEST_EQ(decoded.identity(), request.identity());
  TEST_EQ(decoded.expected_snapshot(), request.expected_snapshot());
  TEST_EQ(decoded.target_binding(), request.target_binding());
  TEST_EQ(decoded.deltas(), request.deltas());
  TEST_EQ(decoded.transaction_evidence(), request.transaction_evidence());
  TEST_EQ(encode_state_publication_request(decoded), encoded);
}

void check_receipt_round_trip(const state_publication_receipt& receipt,
                              const state_publication_request& request,
                              const snapshot& actual_prior)
{
  const state_publication_receipt_encoding encoded =
      encode_state_publication_receipt(receipt);
  const state_publication_receipt decoded =
      decode_state_publication_receipt(encoded, request, actual_prior);
  TEST_EQ(decoded, receipt);
  TEST_EQ(encode_state_publication_receipt(decoded), encoded);
}

template<typename Callable>
bool rejects_with(const state_publication_codec_error_code code,
                  Callable&& callable)
{
  try
  {
    callable();
  }
  catch (const state_publication_codec_error& error)
  {
    return error.code() == code;
  }
  return false;
}

} // namespace

int main()
{
  const state_target_binding binding = native_fixture::target();
  const snapshot empty = snapshot::make(binding);

  const state_publication_request install = install_request(empty);
  check_request_round_trip(install, empty);

  const state_publication_request_encoding current_request =
      encode_state_publication_request(install);
  TEST(starts_with(current_request, request_magic));
  TEST_EQ(current_request[request_magic.size()], 0U);
  TEST_EQ(current_request[request_magic.size() + 1U], 1U);

  auto request_with_receipt_magic = current_request;
  std::copy(receipt_magic.begin(), receipt_magic.end(),
            request_with_receipt_magic.begin());
  replace_checksum(request_with_receipt_magic);
  TEST(rejects_with(
      state_publication_codec_error_code::invalid_magic,
      [&] {
        static_cast<void>(decode_state_publication_request(
            request_with_receipt_magic, empty));
      }));

  const installed_package old_package =
      native_fixture::package("example", 20, binding);
  const snapshot installed = snapshot::make(binding, {old_package});
  installed_package replacement =
      native_fixture::package("example", 60, binding);
  const state_publication_request replace = state_publication_request::make(
      installed,
      {package_state_delta::replace(
          old_package.identity(), replacement,
          replacement.receipt().operation_plan(),
          replacement.receipt().application_evidence())});
  check_request_round_trip(replace, installed);

  const state_publication_request remove = state_publication_request::make(
      installed,
      {package_state_delta::remove(
          old_package.release().name(), old_package.identity(),
          old_package.receipt().operation_plan(),
          old_package.receipt().application_evidence())});
  check_request_round_trip(remove, installed);

  const state_publication_request transactional_remove =
      state_publication_request::make(
          installed,
          {package_state_delta::remove(
              old_package.release().name(), old_package.identity(),
              old_package.receipt().operation_plan(),
              old_package.receipt().application_evidence())},
          native_fixture::identity<transaction_evidence_identity>(170));
  check_request_round_trip(transactional_remove, installed);

  auto corrupt_request = encode_state_publication_request(transactional_remove);
  TEST(corrupt_request.size() > 100);
  corrupt_request[80] ^= 0x80U;
  TEST(rejects_with(
      state_publication_codec_error_code::checksum_mismatch,
      [&] { static_cast<void>(decode_state_publication_request(corrupt_request, empty)); }));

  auto truncated_request = encode_state_publication_request(install);
  truncated_request.pop_back();
  TEST(rejects_with(
      state_publication_codec_error_code::checksum_mismatch,
      [&] { static_cast<void>(decode_state_publication_request(truncated_request, empty)); }));

  const snapshot foreign_expected = snapshot::make(
      binding, {native_fixture::package("foreign", 180, binding)});
  TEST(rejects_with(
      state_publication_codec_error_code::expected_snapshot_mismatch,
      [&] {
        static_cast<void>(decode_state_publication_request(
            encode_state_publication_request(install), foreign_expected));
      }));

  const snapshot result = projected_install(install);
  const std::vector<state_publication_evidence_identity> evidence = {
      native_fixture::identity<state_publication_evidence_identity>(200),
      native_fixture::identity<state_publication_evidence_identity>(210),
  };

  const state_publication_receipt published =
      state_publication_receipt::published(
          install, empty, result, "libpkgstate-generation-v1",
          state_storage_atomicity_boundary::immutable_generation_selection,
          evidence);
  check_receipt_round_trip(published, install, empty);

  const state_publication_receipt_encoding current_receipt =
      encode_state_publication_receipt(published);
  TEST(starts_with(current_receipt, receipt_magic));
  TEST_EQ(current_receipt[receipt_magic.size()], 0U);
  TEST_EQ(current_receipt[receipt_magic.size() + 1U], 1U);

  auto receipt_with_request_magic = current_receipt;
  std::copy(request_magic.begin(), request_magic.end(),
            receipt_with_request_magic.begin());
  replace_checksum(receipt_with_request_magic);
  TEST(rejects_with(
      state_publication_codec_error_code::invalid_magic,
      [&] {
        static_cast<void>(decode_state_publication_receipt(
            receipt_with_request_magic, install, empty));
      }));

  const snapshot stale_prior = native_fixture::state_with_package(
      "other", 220, binding);
  check_receipt_round_trip(
      state_publication_receipt::stale_expected_state(
          install, stale_prior, "libpkgstate-generation-v1", evidence),
      install, stale_prior);

  check_receipt_round_trip(
      state_publication_receipt::request_rejected(
          install, empty, "libpkgstate-generation-v1", evidence),
      install, empty);

  check_receipt_round_trip(
      state_publication_receipt::failed_before_publication(
          install, empty, "libpkgstate-generation-v1", evidence),
      install, empty);

  check_receipt_round_trip(
      state_publication_receipt::published_but_durability_unconfirmed(
          install, empty, result, "libpkgstate-generation-v1",
          state_storage_atomicity_boundary::immutable_generation_selection,
          evidence),
      install, empty);

  check_receipt_round_trip(
      state_publication_receipt::indeterminate(
          install, empty, result.identity(), "libpkgstate-generation-v1",
          state_storage_atomicity_boundary::immutable_generation_selection,
          evidence),
      install, empty);

  check_receipt_round_trip(
      state_publication_receipt::indeterminate(
          install, empty, std::nullopt, "libpkgstate-generation-v1",
          state_storage_atomicity_boundary::immutable_generation_selection,
          evidence),
      install, empty);

  auto corrupt_receipt = encode_state_publication_receipt(
      state_publication_receipt::published(
          install, empty, result, "libpkgstate-generation-v1",
          state_storage_atomicity_boundary::immutable_generation_selection));
  TEST(corrupt_receipt.size() > 100);
  corrupt_receipt[90] ^= 0x01U;
  TEST(rejects_with(
      state_publication_codec_error_code::checksum_mismatch,
      [&] {
        static_cast<void>(decode_state_publication_receipt(
            corrupt_receipt, install, empty));
      }));

  auto truncated_receipt = encode_state_publication_receipt(
      state_publication_receipt::request_rejected(
          install, empty, "libpkgstate-generation-v1"));
  truncated_receipt.pop_back();
  TEST(rejects_with(
      state_publication_codec_error_code::checksum_mismatch,
      [&] {
        static_cast<void>(decode_state_publication_receipt(
            truncated_receipt, install, empty));
      }));

  const state_publication_request foreign_request =
      install_request(empty, "foreign", 230);
  TEST(rejects_with(
      state_publication_codec_error_code::request_mismatch,
      [&] {
        static_cast<void>(decode_state_publication_receipt(
            encode_state_publication_receipt(
                state_publication_receipt::request_rejected(
                    install, empty, "libpkgstate-generation-v1")),
            foreign_request, empty));
      }));

  TEST(rejects_with(
      state_publication_codec_error_code::actual_prior_mismatch,
      [&] {
        static_cast<void>(decode_state_publication_receipt(
            encode_state_publication_receipt(
                state_publication_receipt::request_rejected(
                    install, empty, "libpkgstate-generation-v1")),
            install, stale_prior));
      }));

  return 0;
}
