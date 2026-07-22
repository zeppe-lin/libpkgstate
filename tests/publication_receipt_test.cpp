// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/publication_receipt.h>

#include <optional>
#include <string>
#include <type_traits>
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
      identity<pkgstate::state_store_identity>(static_cast<char>(first + 1)),
      identity<pkgstate::root_view_identity>(static_cast<char>(first + 2)),
      identity<pkgstate::state_backend_identity>(static_cast<char>(first + 3)),
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

} // namespace

int
main()
{
  static_assert(!std::is_same_v<
                pkgstate::state_publication_receipt_identity,
                pkgstate::state_publication_request_identity>);
  static_assert(!std::is_same_v<
                pkgstate::state_publication_evidence_identity,
                pkgstate::application_evidence_identity>);
  static_assert(!std::is_constructible_v<
                pkgstate::state_publication_receipt,
                pkgstate::state_publication_receipt_identity>);

  const pkgstate::state_target_binding target = binding('1');
  const pkgstate::installed_package old_base =
      package("base", "1.0", target, 'a', {file("usr/bin/base")});
  const pkgstate::installed_package new_base =
      package("base", "2.0", target, 'b',
              {file("usr/bin/base"), file("usr/lib/libbase.so")});
  const pkgstate::snapshot actual_prior =
      pkgstate::snapshot::make(target, {old_base});
  const pkgstate::snapshot resulting =
      pkgstate::snapshot::make(target, {new_base});

  const pkgstate::package_state_delta replacement =
      pkgstate::package_state_delta::replace(
          old_base.identity(), new_base,
          identity<pkgstate::operation_plan_identity>('9'),
          identity<pkgstate::application_evidence_identity>('b'));
  const pkgstate::state_publication_request request =
      pkgstate::state_publication_request::make(
          actual_prior, {replacement});

  const auto evidence7 =
      identity<pkgstate::state_publication_evidence_identity>('7');
  const auto evidence8 =
      identity<pkgstate::state_publication_evidence_identity>('8');

  const pkgstate::state_publication_receipt published =
      pkgstate::state_publication_receipt::published(
          request, actual_prior, resulting, "canonical-generation-v1",
          pkgstate::state_storage_atomicity_boundary::
              immutable_generation_selection,
          {evidence8, evidence7});

  CHECK(published.identity().string() ==
        "v1:sha256:"
        "b675ee018acfe3c0a1ab8f3b1447a261"
        "f9bbadf4c9326ea1b6ecdbdb26ff8147");
  CHECK(published.schema_version() == 1);
  CHECK(published.request() == request.identity());
  CHECK(published.expected_prior_snapshot() == actual_prior.identity());
  CHECK(published.actual_prior_snapshot() == actual_prior.identity());
  CHECK(published.target_binding() == target);
  CHECK(published.state_store() == target.state_store());
  CHECK(published.backend() == target.state_backend());
  CHECK(published.storage_format() == "canonical-generation-v1");
  CHECK(published.outcome() ==
        pkgstate::state_publication_outcome::published);
  CHECK(published.durability() ==
        pkgstate::state_publication_durability::confirmed);
  CHECK(published.atomicity_boundary() ==
        pkgstate::state_storage_atomicity_boundary::
            immutable_generation_selection);
  CHECK(published.resulting_snapshot().has_value());
  CHECK(*published.resulting_snapshot() == resulting.identity());
  CHECK(published.subordinate_evidence().size() == 2);
  CHECK(published.subordinate_evidence()[0] == evidence7);
  CHECK(published.subordinate_evidence()[1] == evidence8);

  const pkgstate::state_publication_receipt permuted =
      pkgstate::state_publication_receipt::published(
          request, actual_prior, resulting, "canonical-generation-v1",
          pkgstate::state_storage_atomicity_boundary::
              immutable_generation_selection,
          {evidence7, evidence8});
  CHECK(permuted == published);
  CHECK(permuted.identity() == published.identity());

  const pkgstate::state_publication_receipt changed_format =
      pkgstate::state_publication_receipt::published(
          request, actual_prior, resulting, "canonical-generation-v2",
          pkgstate::state_storage_atomicity_boundary::
              immutable_generation_selection,
          {evidence7, evidence8});
  CHECK(changed_format.identity() != published.identity());

  const pkgstate::snapshot stale_prior =
      pkgstate::snapshot::make(target, {});
  const pkgstate::state_publication_receipt stale =
      pkgstate::state_publication_receipt::stale_expected_state(
          request, stale_prior, "canonical-generation-v1", {evidence7});
  CHECK(stale.outcome() ==
        pkgstate::state_publication_outcome::stale_expected_state);
  CHECK(stale.actual_prior_snapshot() == stale_prior.identity());
  CHECK(stale.durability() ==
        pkgstate::state_publication_durability::not_attempted);
  CHECK(stale.atomicity_boundary() ==
        pkgstate::state_storage_atomicity_boundary::none);
  CHECK(!stale.resulting_snapshot().has_value());

  const pkgstate::state_publication_receipt rejected =
      pkgstate::state_publication_receipt::request_rejected(
          request, actual_prior, "canonical-generation-v1");
  CHECK(rejected.outcome() ==
        pkgstate::state_publication_outcome::request_rejected);
  CHECK(rejected.durability() ==
        pkgstate::state_publication_durability::not_attempted);

  const pkgstate::state_publication_receipt failed =
      pkgstate::state_publication_receipt::failed_before_publication(
          request, actual_prior, "canonical-generation-v1");
  CHECK(failed.outcome() ==
        pkgstate::state_publication_outcome::failed_before_publication);
  CHECK(failed.durability() ==
        pkgstate::state_publication_durability::not_attempted);

  const pkgstate::state_publication_receipt unconfirmed =
      pkgstate::state_publication_receipt::
          published_but_durability_unconfirmed(
              request, actual_prior, resulting, "canonical-generation-v1",
              pkgstate::state_storage_atomicity_boundary::
                  immutable_generation_selection);
  CHECK(unconfirmed.outcome() ==
        pkgstate::state_publication_outcome::
            published_durability_unconfirmed);
  CHECK(unconfirmed.durability() ==
        pkgstate::state_publication_durability::unconfirmed);
  CHECK(unconfirmed.resulting_snapshot().has_value());

  const pkgstate::state_publication_receipt indeterminate =
      pkgstate::state_publication_receipt::indeterminate(
          request, actual_prior, std::nullopt, "canonical-generation-v1",
          pkgstate::state_storage_atomicity_boundary::
              immutable_generation_selection);
  CHECK(indeterminate.outcome() ==
        pkgstate::state_publication_outcome::indeterminate);
  CHECK(indeterminate.durability() ==
        pkgstate::state_publication_durability::indeterminate);
  CHECK(!indeterminate.resulting_snapshot().has_value());

  const pkgstate::state_publication_receipt indeterminate_with_result =
      pkgstate::state_publication_receipt::indeterminate(
          request, actual_prior, resulting.identity(),
          "canonical-generation-v1",
          pkgstate::state_storage_atomicity_boundary::
              immutable_generation_selection);
  CHECK(indeterminate_with_result.resulting_snapshot().has_value());
  CHECK(*indeterminate_with_result.resulting_snapshot() ==
        resulting.identity());

  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::state_publication_receipt::published(
        request, stale_prior, resulting, "canonical-generation-v1",
        pkgstate::state_storage_atomicity_boundary::
            immutable_generation_selection));
  });
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(
        pkgstate::state_publication_receipt::stale_expected_state(
            request, actual_prior, "canonical-generation-v1"));
  });
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::state_publication_receipt::published(
        request, actual_prior, actual_prior, "canonical-generation-v1",
        pkgstate::state_storage_atomicity_boundary::
            immutable_generation_selection));
  });
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::state_publication_receipt::published(
        request, actual_prior, resulting, "canonical-generation-v1",
        pkgstate::state_storage_atomicity_boundary::none));
  });
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::state_publication_receipt::indeterminate(
        request, actual_prior, std::nullopt, "canonical-generation-v1",
        pkgstate::state_storage_atomicity_boundary::none));
  });
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::state_publication_receipt::indeterminate(
        request, actual_prior, actual_prior.identity(),
        "canonical-generation-v1",
        pkgstate::state_storage_atomicity_boundary::
            immutable_generation_selection));
  });
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::state_publication_receipt::request_rejected(
        request, actual_prior, ""));
  });
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::state_publication_receipt::request_rejected(
        request, actual_prior, "bad\nformat"));
  });
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::state_publication_receipt::request_rejected(
        request, actual_prior, "canonical-generation-v1",
        {evidence7, evidence7}));
  });

  const pkgstate::state_target_binding other_target = binding('a');
  const pkgstate::snapshot wrong_target =
      pkgstate::snapshot::make(other_target, {});
  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(
        pkgstate::state_publication_receipt::stale_expected_state(
            request, wrong_target, "canonical-generation-v1"));
  });

  return 0;
}
