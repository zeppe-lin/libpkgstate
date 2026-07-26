// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "native_fixture.h"
#include "test.h"

#include <memory>
#include <string>
#include <utility>

namespace {

class fake_store final : public pkgstate::canonical_store {
public:
  explicit fake_store(pkgstate::snapshot current)
      : current_(std::move(current)) {}

  [[nodiscard]] pkgstate::snapshot read() const override { return current_; }
  [[nodiscard]] std::size_t publications() const noexcept { return publications_; }

private:
  class transaction final : public pkgstate::canonical_publication_transaction {
  public:
    explicit transaction(const fake_store& owner) : owner_(owner) {}
    [[nodiscard]] const pkgstate::snapshot& current() const noexcept override
    { return owner_.current_; }
    [[nodiscard]] const std::string& storage_format() const noexcept override
    { return format_; }
    [[nodiscard]] pkgstate::state_publication_backend_result publish(
        const pkgstate::snapshot& resulting) override
    {
      owner_.current_ = resulting;
      ++owner_.publications_;
      return pkgstate::state_publication_backend_result::published(
          pkgstate::state_storage_atomicity_boundary::
              immutable_generation_selection);
    }
  private:
    const fake_store& owner_;
    std::string format_ = "fake-native-v1";
  };

  [[nodiscard]] std::unique_ptr<pkgstate::canonical_publication_transaction>
  begin_publication() const override
  {
    return std::make_unique<transaction>(*this);
  }

  mutable pkgstate::snapshot current_;
  mutable std::size_t publications_ = 0;
};

} // namespace

int main()
{
  using namespace pkgstate;
  const state_target_binding binding = native_fixture::target();
  const snapshot empty = snapshot::make(binding);
  installed_package proposed = native_fixture::package("example", 20, binding);
  const state_publication_request request = state_publication_request::make(
      empty,
      {package_state_delta::install(
          proposed, proposed.receipt().operation_plan(),
          proposed.receipt().application_evidence())});

  fake_store current(empty);
  const state_publication_receipt published = current.compare_and_publish(request);
  TEST_EQ(published.outcome(), state_publication_outcome::published);
  TEST_EQ(current.publications(), std::size_t{1});
  TEST_EQ(current.read().find_package("example")->identity(), proposed.identity());

  fake_store stale(snapshot::make(binding, {proposed}));
  const state_publication_receipt refused = stale.compare_and_publish(request);
  TEST_EQ(refused.outcome(), state_publication_outcome::stale_expected_state);
  TEST_EQ(stale.publications(), std::size_t{0});
}
