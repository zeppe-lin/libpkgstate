// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../support/publication_codec.h"

#include <libpkgstate/publication_projection.h>

int main()
{
  using namespace pkgstate;
  using namespace publication_codec_fixture;
  const state_target_binding binding = state_fixture::target();
  const snapshot empty = snapshot::make(binding);
  const state_publication_request request = state_fixture::install_request(empty);
  const snapshot result = project_publication_request(request, empty);
  const std::vector<state_publication_evidence_identity> evidence = {
      state_fixture::identity<state_publication_evidence_identity>(200),
      state_fixture::identity<state_publication_evidence_identity>(210)};

  const state_publication_receipt published = state_publication_receipt::published(
      request, empty, result, "libpkgstate-generation-v1",
      state_storage_atomicity_boundary::immutable_generation_selection, evidence);
  check_receipt_round_trip(published, request, empty);

  const auto current = encode_state_publication_receipt(published);
  TEST(starts_with(current, receipt_magic));
  TEST_EQ(current[receipt_magic.size()], 0U);
  TEST_EQ(current[receipt_magic.size() + 1U], 1U);

  auto wrong_magic = current;
  std::copy(request_magic.begin(), request_magic.end(), wrong_magic.begin());
  replace_checksum(wrong_magic);
  TEST(rejects_with(state_publication_codec_error_code::invalid_magic, [&] {
    static_cast<void>(decode_state_publication_receipt(wrong_magic, request, empty));
  }));

  const snapshot stale_prior = state_fixture::state_with_package("other", 220, binding);
  check_receipt_round_trip(
      state_publication_receipt::stale_expected_state(
          request, stale_prior, "libpkgstate-generation-v1", evidence),
      request, stale_prior);
  check_receipt_round_trip(
      state_publication_receipt::request_rejected(
          request, empty, "libpkgstate-generation-v1", evidence),
      request, empty);
  check_receipt_round_trip(
      state_publication_receipt::failed_before_publication(
          request, empty, "libpkgstate-generation-v1", evidence),
      request, empty);
  check_receipt_round_trip(
      state_publication_receipt::published_but_durability_unconfirmed(
          request, empty, result, "libpkgstate-generation-v1",
          state_storage_atomicity_boundary::immutable_generation_selection, evidence),
      request, empty);
  check_receipt_round_trip(
      state_publication_receipt::indeterminate(
          request, empty, result.identity(), "libpkgstate-generation-v1",
          state_storage_atomicity_boundary::immutable_generation_selection, evidence),
      request, empty);
  check_receipt_round_trip(
      state_publication_receipt::indeterminate(
          request, empty, std::nullopt, "libpkgstate-generation-v1",
          state_storage_atomicity_boundary::immutable_generation_selection, evidence),
      request, empty);

  auto corrupt = current;
  TEST(corrupt.size() > 100);
  corrupt[90] ^= 0x01U;
  TEST(rejects_with(state_publication_codec_error_code::checksum_mismatch, [&] {
    static_cast<void>(decode_state_publication_receipt(corrupt, request, empty));
  }));

  auto truncated = encode_state_publication_receipt(
      state_publication_receipt::request_rejected(
          request, empty, "libpkgstate-generation-v1"));
  truncated.pop_back();
  TEST(rejects_with(state_publication_codec_error_code::checksum_mismatch, [&] {
    static_cast<void>(decode_state_publication_receipt(truncated, request, empty));
  }));

  const state_publication_request foreign_request =
      state_fixture::install_request(empty, "foreign", 230);
  TEST(rejects_with(state_publication_codec_error_code::request_mismatch, [&] {
    static_cast<void>(decode_state_publication_receipt(current, foreign_request, empty));
  }));
  TEST(rejects_with(state_publication_codec_error_code::actual_prior_mismatch, [&] {
    static_cast<void>(decode_state_publication_receipt(current, request, stale_prior));
  }));
}
