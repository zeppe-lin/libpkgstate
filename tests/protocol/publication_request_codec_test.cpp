// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../support/publication_codec.h"

int main()
{
  using namespace pkgstate;
  using namespace publication_codec_fixture;
  const state_target_binding binding = state_fixture::target();
  const snapshot empty = snapshot::make(binding);
  const state_publication_request install = state_fixture::install_request(empty);
  check_request_round_trip(install, empty);

  const auto current = encode_state_publication_request(install);
  TEST(starts_with(current, request_magic));
  TEST_EQ(current[request_magic.size()], 0U);
  TEST_EQ(current[request_magic.size() + 1U], 1U);

  auto wrong_magic = current;
  std::copy(receipt_magic.begin(), receipt_magic.end(), wrong_magic.begin());
  replace_checksum(wrong_magic);
  TEST(rejects_with(state_publication_codec_error_code::invalid_magic, [&] {
    static_cast<void>(decode_state_publication_request(wrong_magic, empty));
  }));

  const installed_package old_package = state_fixture::package("example", 20, binding);
  const snapshot installed = snapshot::make(binding, {old_package});
  const installed_package replacement = state_fixture::package("example", 60, binding);
  const state_publication_request replace = state_publication_request::make(
      installed,
      {package_state_delta::replace(
          old_package.identity(), replacement, replacement.receipt().operation_plan(),
          replacement.receipt().application_evidence())});
  check_request_round_trip(replace, installed);

  const state_publication_request remove = state_publication_request::make(
      installed,
      {package_state_delta::remove(
          old_package.release().name(), old_package.identity(),
          old_package.receipt().operation_plan(),
          old_package.receipt().application_evidence())});
  check_request_round_trip(remove, installed);

  const auto transaction = state_fixture::identity<transaction_evidence_identity>(170);
  const installed_package second = state_fixture::package("second", 80, binding, transaction);
  const state_publication_request composed = state_publication_request::make(
      installed,
      {package_state_delta::remove(
           old_package.release().name(), old_package.identity(),
           old_package.receipt().operation_plan(),
           old_package.receipt().application_evidence()),
       state_fixture::install_delta(second)},
      transaction);
  check_request_round_trip(composed, installed);

  auto corrupt = encode_state_publication_request(composed);
  TEST(corrupt.size() > 100);
  corrupt[80] ^= 0x80U;
  TEST(rejects_with(state_publication_codec_error_code::checksum_mismatch, [&] {
    static_cast<void>(decode_state_publication_request(corrupt, installed));
  }));

  auto truncated = current;
  truncated.pop_back();
  TEST(rejects_with(state_publication_codec_error_code::checksum_mismatch, [&] {
    static_cast<void>(decode_state_publication_request(truncated, empty));
  }));

  const snapshot foreign_expected = state_fixture::state_with_package("foreign", 180, binding);
  TEST(rejects_with(
      state_publication_codec_error_code::expected_snapshot_mismatch, [&] {
        static_cast<void>(decode_state_publication_request(current, foreign_expected));
      }));
}
