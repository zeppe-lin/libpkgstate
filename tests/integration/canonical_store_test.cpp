// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/state.h"
#include "../support/test.h"

#include <libpkgstate/error.h>
#include <libpkgstate/publication_projection.h>

#include <memory>
#include <string>
#include <utility>

namespace {

class fake_store final : public pkgstate::canonical_store {
public:
  fake_store(pkgstate::snapshot current,
             pkgstate::state_publication_backend_result result,
             std::string format = "fake-native-v1",
             bool null_transaction = false)
      : current_(std::move(current)), result_(std::move(result)),
        format_(std::move(format)), null_transaction_(null_transaction) {}

  [[nodiscard]] pkgstate::snapshot read() const override { return current_; }
  [[nodiscard]] std::size_t publications() const noexcept { return publications_; }
  [[nodiscard]] const std::optional<pkgstate::installed_state_snapshot_identity>&
  published_identity() const noexcept { return published_identity_; }

private:
  class transaction final : public pkgstate::canonical_publication_transaction {
  public:
    explicit transaction(const fake_store& owner) : owner_(owner) {}
    [[nodiscard]] const pkgstate::snapshot& current() const noexcept override
    { return owner_.current_; }
    [[nodiscard]] const std::string& storage_format() const noexcept override
    { return owner_.format_; }
    [[nodiscard]] pkgstate::state_publication_backend_result publish(
        const pkgstate::snapshot& resulting) override
    {
      owner_.published_identity_ = resulting.identity();
      ++owner_.publications_;
      if (owner_.result_.resulting_snapshot_established())
        owner_.current_ = resulting;
      return owner_.result_;
    }
  private:
    const fake_store& owner_;
  };

  [[nodiscard]] std::unique_ptr<pkgstate::canonical_publication_transaction>
  begin_publication() const override
  {
    if (null_transaction_)
      return nullptr;
    return std::make_unique<transaction>(*this);
  }

  mutable pkgstate::snapshot current_;
  pkgstate::state_publication_backend_result result_;
  std::string format_;
  bool null_transaction_;
  mutable std::size_t publications_ = 0;
  mutable std::optional<pkgstate::installed_state_snapshot_identity> published_identity_;
};

} // namespace

int main()
{
  using namespace pkgstate;
  const state_target_binding binding = state_fixture::target();
  const snapshot empty = snapshot::make(binding);
  const state_publication_request request = state_fixture::install_request(empty);
  const snapshot expected_result = project_publication_request(request, empty);
  const auto evidence = state_fixture::identity<state_publication_evidence_identity>(90);

  fake_store published(
      empty, state_publication_backend_result::published(
                 state_storage_atomicity_boundary::immutable_generation_selection,
                 {evidence}));
  const state_publication_receipt published_receipt =
      published.compare_and_publish(request);
  TEST_EQ(published_receipt.outcome(), state_publication_outcome::published);
  TEST_EQ(published.publications(), std::size_t{1});
  TEST_EQ(*published.published_identity(), expected_result.identity());
  TEST_EQ(published.read().identity(), expected_result.identity());
  TEST_EQ(published_receipt.subordinate_evidence().front(), evidence);

  const snapshot changed = state_fixture::state_with_package("other", 40, binding);
  fake_store stale(
      changed, state_publication_backend_result::published(
                   state_storage_atomicity_boundary::immutable_generation_selection));
  const state_publication_receipt stale_receipt = stale.compare_and_publish(request);
  TEST_EQ(stale_receipt.outcome(), state_publication_outcome::stale_expected_state);
  TEST_EQ(stale.publications(), std::size_t{0});

  fake_store rejected(empty, state_publication_backend_result::request_rejected());
  TEST_EQ(rejected.compare_and_publish(request).outcome(),
          state_publication_outcome::request_rejected);
  TEST_EQ(rejected.publications(), std::size_t{1});

  fake_store failed(empty, state_publication_backend_result::failed_before_publication());
  TEST_EQ(failed.compare_and_publish(request).outcome(),
          state_publication_outcome::failed_before_publication);

  fake_store unconfirmed(
      empty, state_publication_backend_result::published_but_durability_unconfirmed(
                 state_storage_atomicity_boundary::complete_state_object_replace));
  const auto unconfirmed_receipt = unconfirmed.compare_and_publish(request);
  TEST_EQ(unconfirmed_receipt.outcome(),
          state_publication_outcome::published_durability_unconfirmed);
  TEST_EQ(*unconfirmed_receipt.resulting_snapshot(), expected_result.identity());

  fake_store indeterminate_unknown(
      empty, state_publication_backend_result::indeterminate(
                 state_storage_atomicity_boundary::immutable_generation_selection,
                 false));
  TEST(!indeterminate_unknown.compare_and_publish(request).resulting_snapshot());

  fake_store indeterminate_established(
      empty, state_publication_backend_result::indeterminate(
                 state_storage_atomicity_boundary::immutable_generation_selection,
                 true));
  TEST_EQ(*indeterminate_established.compare_and_publish(request).resulting_snapshot(),
          expected_result.identity());

  fake_store no_transaction(
      empty, state_publication_backend_result::request_rejected(),
      "fake-native-v1", true);
  TEST_THROWS(store_error, no_transaction.compare_and_publish(request));

  fake_store bad_format(
      empty, state_publication_backend_result::request_rejected(), "bad\nformat");
  TEST_THROWS(state_error, bad_format.compare_and_publish(request));

  fake_store foreign(
      snapshot::make(state_fixture::target(130)),
      state_publication_backend_result::request_rejected());
  TEST_THROWS(store_error, foreign.compare_and_publish(request));

  TEST_THROWS(state_error,
              state_publication_backend_result::published(
                  state_storage_atomicity_boundary::none));
  TEST_THROWS(state_error,
              state_publication_backend_result::indeterminate(
                  state_storage_atomicity_boundary::none, false));
  TEST_THROWS(state_error,
              state_publication_backend_result::request_rejected(
                  {evidence, evidence}));
}
