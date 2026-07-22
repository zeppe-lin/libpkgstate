// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/libpkgstate.h>

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "test.h"

namespace {

template<typename Identity>
Identity
identity(char digit)
{
  return Identity::parse("v1:sha256:" + std::string(64, digit));
}

pkgstate::state_target_binding
binding(char first)
{
  return pkgstate::state_target_binding::make(
      identity<pkgstate::managed_target_identity>(first),
      identity<pkgstate::state_store_identity>(
          static_cast<char>(first + 1)),
      identity<pkgstate::root_view_identity>(
          static_cast<char>(first + 2)),
      identity<pkgstate::state_backend_identity>(
          static_cast<char>(first + 3)),
      identity<pkgstate::publication_domain_identity>(
          static_cast<char>(first + 4)));
}

pkgstate::owned_entry
file(const char* value)
{
  return {pkgstate::package_path::parse(value),
          pkgstate::owned_entry_type::non_directory};
}

pkgstate::installed_package
package(const char* name,
        const char* version,
        const pkgstate::state_target_binding& target,
        char application,
        std::vector<pkgstate::owned_entry> manifest)
{
  const pkgstate::package_release release =
      pkgstate::package_release::make(name, version, "1");
  const pkgstate::installed_control control =
      pkgstate::installed_control::make(
          release, {}, {}, {}, {},
          {pkgstate::control_provenance::make(
              pkgstate::control_provenance_kind::application_evidence,
              identity<pkgstate::application_evidence_identity>(application)
                  .string())});
  return pkgstate::installed_package::make(
      release, control, target, std::move(manifest));
}

pkgstate::state_publication_request
install_request(const pkgstate::snapshot& expected,
                const pkgstate::installed_package& proposed,
                char plan,
                char application)
{
  return pkgstate::state_publication_request::make(
      expected,
      {pkgstate::package_state_delta::install(
          proposed, identity<pkgstate::operation_plan_identity>(plan),
          identity<pkgstate::application_evidence_identity>(application))});
}

enum class fake_outcome {
  published,
  rejected,
  failed,
  unconfirmed,
  indeterminate_unknown,
  indeterminate_established,
};

class fake_store final : public pkgstate::canonical_store {
public:
  fake_store(pkgstate::snapshot current,
             fake_outcome outcome = fake_outcome::published,
             std::string storage_format = "fake-generation-v1")
      : current_(std::move(current)),
        outcome_(outcome),
        storage_format_(std::move(storage_format))
  {
  }

  [[nodiscard]] pkgstate::snapshot read() const override
  {
    ++read_count_;
    return current_;
  }

  void outcome(fake_outcome value) noexcept
  {
    outcome_ = value;
  }

  [[nodiscard]] std::size_t begin_count() const noexcept
  {
    return begin_count_;
  }

  [[nodiscard]] std::size_t publish_count() const noexcept
  {
    return publish_count_;
  }

  [[nodiscard]] std::size_t read_count() const noexcept
  {
    return read_count_;
  }

  [[nodiscard]] bool observed_locked_publish() const noexcept
  {
    return observed_locked_publish_;
  }

private:
  class transaction final
      : public pkgstate::canonical_publication_transaction {
  public:
    explicit transaction(const fake_store& owner)
        : owner_(owner)
    {
      if (owner_.locked_)
        throw pkgstate::store_error("fake store publication lock is busy");
      owner_.locked_ = true;
    }

    ~transaction() override
    {
      owner_.locked_ = false;
    }

    [[nodiscard]] const pkgstate::snapshot&
    current() const noexcept override
    {
      return owner_.current_;
    }

    [[nodiscard]] const std::string&
    storage_format() const noexcept override
    {
      return owner_.storage_format_;
    }

    [[nodiscard]] pkgstate::state_publication_backend_result
    publish(const pkgstate::snapshot& resulting_snapshot) override
    {
      ++owner_.publish_count_;
      owner_.observed_locked_publish_ = owner_.locked_;

      const auto evidence =
          identity<pkgstate::state_publication_evidence_identity>('e');
      const auto boundary =
          pkgstate::state_storage_atomicity_boundary::
              immutable_generation_selection;

      switch (owner_.outcome_)
      {
        case fake_outcome::published:
          owner_.current_ = resulting_snapshot;
          return pkgstate::state_publication_backend_result::published(
              boundary, {evidence});

        case fake_outcome::rejected:
          return pkgstate::state_publication_backend_result::
              request_rejected({evidence});

        case fake_outcome::failed:
          return pkgstate::state_publication_backend_result::
              failed_before_publication({evidence});

        case fake_outcome::unconfirmed:
          owner_.current_ = resulting_snapshot;
          return pkgstate::state_publication_backend_result::
              published_but_durability_unconfirmed(boundary, {evidence});

        case fake_outcome::indeterminate_unknown:
          return pkgstate::state_publication_backend_result::indeterminate(
              boundary, false, {evidence});

        case fake_outcome::indeterminate_established:
          owner_.current_ = resulting_snapshot;
          return pkgstate::state_publication_backend_result::indeterminate(
              boundary, true, {evidence});
      }

      throw pkgstate::state_error("invalid fake publication outcome");
    }

  private:
    const fake_store& owner_;
  };

  [[nodiscard]] std::unique_ptr<pkgstate::canonical_publication_transaction>
  begin_publication() const override
  {
    ++begin_count_;
    return std::make_unique<transaction>(*this);
  }

  mutable pkgstate::snapshot current_;
  mutable fake_outcome outcome_;
  std::string storage_format_;
  mutable bool locked_ = false;
  mutable bool observed_locked_publish_ = false;
  mutable std::size_t begin_count_ = 0;
  mutable std::size_t publish_count_ = 0;
  mutable std::size_t read_count_ = 0;
};

class null_store final : public pkgstate::canonical_store {
public:
  explicit null_store(pkgstate::snapshot current)
      : current_(std::move(current))
  {
  }

  [[nodiscard]] pkgstate::snapshot read() const override
  {
    return current_;
  }

private:
  [[nodiscard]] std::unique_ptr<pkgstate::canonical_publication_transaction>
  begin_publication() const override
  {
    return nullptr;
  }

  pkgstate::snapshot current_;
};

} // namespace

int
main()
{
  static_assert(!std::is_base_of_v<pkgstate::store,
                                   pkgstate::canonical_store>);

  const pkgstate::state_target_binding target = binding('1');
  const pkgstate::snapshot empty = pkgstate::snapshot::make(target, {});
  const pkgstate::installed_package base1 =
      package("base", "1.0", target, 'b', {file("usr/bin/base")});
  const pkgstate::state_publication_request install =
      install_request(empty, base1, 'a', 'b');
  const pkgstate::snapshot with_base =
      pkgstate::snapshot::make(target, {base1});

  fake_store store(empty);
  const pkgstate::state_publication_receipt installed =
      store.compare_and_publish(install);
  CHECK(installed.outcome() ==
        pkgstate::state_publication_outcome::published);
  CHECK(installed.resulting_snapshot().has_value());
  CHECK(*installed.resulting_snapshot() == with_base.identity());
  CHECK(store.read().identity() == with_base.identity());
  CHECK(store.begin_count() == 1);
  CHECK(store.publish_count() == 1);
  CHECK(store.read_count() == 1);
  CHECK(store.observed_locked_publish());

  const auto evidence =
      identity<pkgstate::state_publication_evidence_identity>('e');
  const pkgstate::state_publication_receipt direct =
      pkgstate::state_publication_receipt::published(
          install, empty, with_base, "fake-generation-v1",
          pkgstate::state_storage_atomicity_boundary::
              immutable_generation_selection,
          {evidence});
  CHECK(installed == direct);

  const pkgstate::installed_package base2 =
      package("base", "2.0", target, 'd',
              {file("usr/bin/base"), file("usr/lib/libbase.so")});
  const pkgstate::state_publication_request replace =
      pkgstate::state_publication_request::make(
          with_base,
          {pkgstate::package_state_delta::replace(
              base1.identity(), base2,
              identity<pkgstate::operation_plan_identity>('c'),
              identity<pkgstate::application_evidence_identity>('d'))});
  const pkgstate::state_publication_receipt replaced =
      store.compare_and_publish(replace);
  CHECK(replaced.outcome() ==
        pkgstate::state_publication_outcome::published);
  CHECK(store.read().find_package("base")->identity() == base2.identity());

  const pkgstate::snapshot with_base2 = store.read();
  const pkgstate::state_publication_request remove =
      pkgstate::state_publication_request::make(
          with_base2,
          {pkgstate::package_state_delta::remove(
              "base", base2.identity(),
              identity<pkgstate::operation_plan_identity>('5'),
              identity<pkgstate::application_evidence_identity>('6'))});
  const pkgstate::state_publication_receipt removed =
      store.compare_and_publish(remove);
  CHECK(removed.outcome() ==
        pkgstate::state_publication_outcome::published);
  CHECK(store.read().size() == 0);

  fake_store stale(with_base);
  const pkgstate::state_publication_receipt stale_receipt =
      stale.compare_and_publish(install);
  CHECK(stale_receipt.outcome() ==
        pkgstate::state_publication_outcome::stale_expected_state);
  CHECK(stale_receipt.actual_prior_snapshot() == with_base.identity());
  CHECK(stale.publish_count() == 0);
  CHECK(stale.read().identity() == with_base.identity());

  for (const auto& [backend_outcome, expected_outcome] : {
           std::pair{fake_outcome::rejected,
                     pkgstate::state_publication_outcome::request_rejected},
           std::pair{fake_outcome::failed,
                     pkgstate::state_publication_outcome::
                         failed_before_publication},
       })
  {
    fake_store unchanged(empty, backend_outcome);
    const pkgstate::state_publication_receipt receipt =
        unchanged.compare_and_publish(install);
    CHECK(receipt.outcome() == expected_outcome);
    CHECK(!receipt.resulting_snapshot().has_value());
    CHECK(unchanged.read().identity() == empty.identity());
  }

  fake_store unconfirmed(empty, fake_outcome::unconfirmed);
  const pkgstate::state_publication_receipt unconfirmed_receipt =
      unconfirmed.compare_and_publish(install);
  CHECK(unconfirmed_receipt.outcome() ==
        pkgstate::state_publication_outcome::
            published_durability_unconfirmed);
  CHECK(unconfirmed_receipt.resulting_snapshot().has_value());
  CHECK(unconfirmed.read().identity() == with_base.identity());

  fake_store unknown(empty, fake_outcome::indeterminate_unknown);
  const pkgstate::state_publication_receipt unknown_receipt =
      unknown.compare_and_publish(install);
  CHECK(unknown_receipt.outcome() ==
        pkgstate::state_publication_outcome::indeterminate);
  CHECK(!unknown_receipt.resulting_snapshot().has_value());
  CHECK(unknown.read().identity() == empty.identity());

  fake_store established(empty, fake_outcome::indeterminate_established);
  const pkgstate::state_publication_receipt established_receipt =
      established.compare_and_publish(install);
  CHECK(established_receipt.outcome() ==
        pkgstate::state_publication_outcome::indeterminate);
  CHECK(established_receipt.resulting_snapshot().has_value());
  CHECK(*established_receipt.resulting_snapshot() == with_base.identity());
  CHECK(established.read().identity() == with_base.identity());

  const auto duplicate_evidence =
      identity<pkgstate::state_publication_evidence_identity>('7');
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::state_publication_backend_result::published(
        pkgstate::state_storage_atomicity_boundary::none));
  });
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::state_publication_backend_result::
        request_rejected({duplicate_evidence, duplicate_evidence}));
  });

  fake_store wrong_target(pkgstate::snapshot::make(binding('a'), {}));
  check_throws<pkgstate::store_error>([&] {
    static_cast<void>(wrong_target.compare_and_publish(install));
  });

  null_store null_backend(empty);
  check_throws<pkgstate::store_error>([&] {
    static_cast<void>(null_backend.compare_and_publish(install));
  });

  fake_store invalid_format(empty, fake_outcome::published, "bad\nformat");
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(invalid_format.compare_and_publish(install));
  });
  CHECK(invalid_format.publish_count() == 0);
  CHECK(invalid_format.read().identity() == empty.identity());

  return 0;
}
