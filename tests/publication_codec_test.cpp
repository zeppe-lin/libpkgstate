// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "native_fixture.h"
#include "test.h"

#include <libpkgstate/publication_codec.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace pkgstate;

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

  check_receipt_round_trip(
      state_publication_receipt::published(
          install, empty, result, "libpkgstate-generation-v3",
          state_storage_atomicity_boundary::immutable_generation_selection,
          evidence),
      install, empty);

  const snapshot stale_prior = native_fixture::state_with_package(
      "other", 220, binding);
  check_receipt_round_trip(
      state_publication_receipt::stale_expected_state(
          install, stale_prior, "libpkgstate-generation-v3", evidence),
      install, stale_prior);

  check_receipt_round_trip(
      state_publication_receipt::request_rejected(
          install, empty, "libpkgstate-generation-v3", evidence),
      install, empty);

  check_receipt_round_trip(
      state_publication_receipt::failed_before_publication(
          install, empty, "libpkgstate-generation-v3", evidence),
      install, empty);

  check_receipt_round_trip(
      state_publication_receipt::published_but_durability_unconfirmed(
          install, empty, result, "libpkgstate-generation-v3",
          state_storage_atomicity_boundary::immutable_generation_selection,
          evidence),
      install, empty);

  check_receipt_round_trip(
      state_publication_receipt::indeterminate(
          install, empty, result.identity(), "libpkgstate-generation-v3",
          state_storage_atomicity_boundary::immutable_generation_selection,
          evidence),
      install, empty);

  check_receipt_round_trip(
      state_publication_receipt::indeterminate(
          install, empty, std::nullopt, "libpkgstate-generation-v3",
          state_storage_atomicity_boundary::immutable_generation_selection,
          evidence),
      install, empty);

  auto corrupt_receipt = encode_state_publication_receipt(
      state_publication_receipt::published(
          install, empty, result, "libpkgstate-generation-v3",
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
          install, empty, "libpkgstate-generation-v3"));
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
                    install, empty, "libpkgstate-generation-v3")),
            foreign_request, empty));
      }));

  TEST(rejects_with(
      state_publication_codec_error_code::actual_prior_mismatch,
      [&] {
        static_cast<void>(decode_state_publication_receipt(
            encode_state_publication_receipt(
                state_publication_receipt::request_rejected(
                    install, empty, "libpkgstate-generation-v3")),
            install, stale_prior));
      }));

  return 0;
}
