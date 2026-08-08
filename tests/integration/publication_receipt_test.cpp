// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/state.h"
#include "../support/test.h"

#include <libpkgstate/error.h>
#include <libpkgstate/publication_projection.h>

int main()
{
  using namespace pkgstate;
  const state_target_binding binding = state_fixture::target();
  const snapshot prior = snapshot::make(binding);
  const state_publication_request request = state_fixture::install_request(prior);
  const snapshot result = project_publication_request(request, prior);
  const auto evidence_a = state_fixture::identity<state_publication_evidence_identity>(90);
  const auto evidence_b = state_fixture::identity<state_publication_evidence_identity>(91);

  const state_publication_receipt published = state_publication_receipt::published(
      request, prior, result, "pkgstate-generation/1",
      state_storage_atomicity_boundary::immutable_generation_selection,
      {evidence_b, evidence_a});
  TEST_EQ(published.outcome(), state_publication_outcome::published);
  TEST_EQ(published.durability(), state_publication_durability::confirmed);
  TEST_EQ(published.atomicity_boundary(),
          state_storage_atomicity_boundary::immutable_generation_selection);
  TEST_EQ(*published.resulting_snapshot(), result.identity());
  TEST_EQ(published.subordinate_evidence().front(), evidence_a);
  TEST_EQ(published.subordinate_evidence().back(), evidence_b);

  const snapshot changed = state_fixture::state_with_package("other", 40, binding);
  const state_publication_receipt stale =
      state_publication_receipt::stale_expected_state(
          request, changed, "pkgstate-generation/1");
  TEST_EQ(stale.outcome(), state_publication_outcome::stale_expected_state);
  TEST_EQ(stale.durability(), state_publication_durability::not_attempted);
  TEST_EQ(stale.atomicity_boundary(), state_storage_atomicity_boundary::none);
  TEST(!stale.resulting_snapshot());

  const state_publication_receipt rejected =
      state_publication_receipt::request_rejected(
          request, prior, "pkgstate-generation/1", {evidence_a});
  TEST_EQ(rejected.outcome(), state_publication_outcome::request_rejected);
  TEST_EQ(rejected.durability(), state_publication_durability::not_attempted);
  TEST(!rejected.resulting_snapshot());

  const state_publication_receipt failed =
      state_publication_receipt::failed_before_publication(
          request, prior, "pkgstate-generation/1");
  TEST_EQ(failed.outcome(), state_publication_outcome::failed_before_publication);
  TEST_EQ(failed.durability(), state_publication_durability::not_attempted);

  const state_publication_receipt unconfirmed =
      state_publication_receipt::published_but_durability_unconfirmed(
          request, prior, result, "pkgstate-generation/1",
          state_storage_atomicity_boundary::complete_state_object_replace);
  TEST_EQ(unconfirmed.outcome(),
          state_publication_outcome::published_durability_unconfirmed);
  TEST_EQ(unconfirmed.durability(), state_publication_durability::unconfirmed);
  TEST_EQ(*unconfirmed.resulting_snapshot(), result.identity());

  const state_publication_receipt indeterminate_unknown =
      state_publication_receipt::indeterminate(
          request, prior, std::nullopt, "pkgstate-generation/1",
          state_storage_atomicity_boundary::immutable_generation_selection);
  TEST_EQ(indeterminate_unknown.outcome(), state_publication_outcome::indeterminate);
  TEST_EQ(indeterminate_unknown.durability(),
          state_publication_durability::indeterminate);
  TEST(!indeterminate_unknown.resulting_snapshot());

  const state_publication_receipt indeterminate_established =
      state_publication_receipt::indeterminate(
          request, prior, result.identity(), "pkgstate-generation/1",
          state_storage_atomicity_boundary::immutable_generation_selection);
  TEST_EQ(*indeterminate_established.resulting_snapshot(), result.identity());

  TEST_THROWS(state_error,
              state_publication_receipt::published(
                  request, prior, result, "pkgstate-generation/1",
                  state_storage_atomicity_boundary::none));
  TEST_THROWS(state_error,
              state_publication_receipt::published(
                  request, changed, result, "pkgstate-generation/1",
                  state_storage_atomicity_boundary::immutable_generation_selection));
  TEST_THROWS(state_error,
              state_publication_receipt::stale_expected_state(
                  request, prior, "pkgstate-generation/1"));
  TEST_THROWS(state_error,
              state_publication_receipt::request_rejected(
                  request, prior, "bad\nformat"));
  TEST_THROWS(state_error,
              state_publication_receipt::request_rejected(
                  request, prior, "pkgstate-generation/1", {evidence_a, evidence_a}));
  TEST_THROWS(state_error,
              state_publication_receipt::indeterminate(
                  request, prior,
                  state_fixture::identity<installed_state_snapshot_identity>(120),
                  "pkgstate-generation/1",
                  state_storage_atomicity_boundary::immutable_generation_selection));
}
