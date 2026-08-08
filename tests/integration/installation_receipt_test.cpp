// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../fixtures/state.h"
#include "../support/test.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <libpkgstate/error.h>
#include <libpkgstate/installation_receipt.h>

int main()
{
  using namespace pkgstate;

  const installation_receipt receipt = state_fixture::receipt();
  TEST_EQ(receipt.schema_version(), installation_receipt_schema_version);
  TEST_EQ(receipt.manifest().size(), std::size_t{2});
  TEST_EQ(receipt.manifest().front().path().string(),
          std::string("etc/example.conf"));
  TEST_EQ(receipt.manifest().back().path().string(),
          std::string("usr/bin/example"));
  TEST(receipt.manifest().front().rejected().has_value());
  TEST_EQ(receipt.control().source().snapshot(),
          state_fixture::source().snapshot());
  TEST_EQ(receipt.target_binding(), state_fixture::target());
  TEST_EQ(receipt.operation_plan(),
          state_fixture::identity<operation_plan_identity>(38));
  TEST_EQ(receipt.application_evidence(),
          state_fixture::identity<application_evidence_identity>(39));
  TEST(!receipt.transaction_evidence());

  const installation_receipt empty = state_fixture::empty_receipt();
  TEST(empty.manifest().empty());
  TEST_NE(empty.identity(), receipt.identity());
  TEST_EQ(empty.release(), receipt.release());

  const transaction_evidence_identity transaction =
      state_fixture::identity<transaction_evidence_identity>(90);
  const installation_receipt transactional = state_fixture::receipt(
      "example", 20, state_fixture::target(), transaction);
  TEST_EQ(*transactional.transaction_evidence(), transaction);
  TEST_NE(transactional.identity(), receipt.identity());

  std::vector<owned_entry> unordered = state_fixture::rich_manifest();
  std::reverse(unordered.begin(), unordered.end());
  const installation_receipt normalized = installation_receipt::make(
      state_fixture::control(), state_fixture::target(), unordered,
      state_fixture::identity<operation_plan_identity>(38),
      state_fixture::identity<application_evidence_identity>(39));
  TEST(std::is_sorted(
      normalized.manifest().begin(), normalized.manifest().end(),
      [](const owned_entry& lhs, const owned_entry& rhs) {
        return lhs.path() < rhs.path();
      }));
  const installation_receipt canonical = installation_receipt::make(
      state_fixture::control(), state_fixture::target(),
      state_fixture::rich_manifest(),
      state_fixture::identity<operation_plan_identity>(38),
      state_fixture::identity<application_evidence_identity>(39));
  TEST_EQ(normalized, canonical);
  TEST_EQ(normalized.identity(), canonical.identity());

  const owned_entry duplicate = state_fixture::entry(
      "same", state_fixture::regular_object(80));
  TEST_THROWS(
      state_error,
      installation_receipt::make(
          state_fixture::control(), state_fixture::target(),
          {duplicate, duplicate},
          state_fixture::identity<operation_plan_identity>(90),
          state_fixture::identity<application_evidence_identity>(91)));

  const installed_object_metadata link_peer = state_fixture::regular_object(
      92, 0644, 1, package_path::parse("usr/lib/anchor"));
  const installation_receipt relation = installation_receipt::make(
      state_fixture::control(), state_fixture::target(),
      {state_fixture::entry("usr/lib/link", link_peer)},
      state_fixture::identity<operation_plan_identity>(93),
      state_fixture::identity<application_evidence_identity>(94));
  TEST_EQ(relation.manifest().front().object().hardlink_anchor()->string(),
          std::string("usr/lib/anchor"));
}
