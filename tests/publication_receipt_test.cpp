// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "native_fixture.h"
#include "test.h"

#include <libpkgstate/publication_receipt.h>
#include <libpkgstate/publication_request.h>

namespace {

pkgstate::state_publication_request install_request(
    const pkgstate::snapshot& prior)
{
  pkgstate::installed_package package = native_fixture::package(
      "example", 20, prior.target_binding());
  const pkgstate::operation_plan_identity operation_plan =
      package.receipt().operation_plan();
  const pkgstate::application_evidence_identity application_evidence =
      package.receipt().application_evidence();
  return pkgstate::state_publication_request::make(
      prior,
      {pkgstate::package_state_delta::install(
          std::move(package), std::move(operation_plan),
          std::move(application_evidence))});
}

} // namespace

int main()
{
  const pkgstate::state_target_binding target = native_fixture::target();
  const pkgstate::snapshot prior = pkgstate::snapshot::make(target);
  const pkgstate::state_publication_request request = install_request(prior);
  const pkgstate::snapshot result = pkgstate::snapshot::make(
      target, {*request.deltas().front().proposed_package()});

  const pkgstate::state_publication_receipt published =
      pkgstate::state_publication_receipt::published(
          request, prior, result, "pkgstate-generation/2",
          pkgstate::state_storage_atomicity_boundary::
              immutable_generation_selection);
  TEST_EQ(published.outcome(), pkgstate::state_publication_outcome::published);
  TEST_EQ(published.durability(),
          pkgstate::state_publication_durability::confirmed);
  TEST_EQ(*published.resulting_snapshot(), result.identity());

  const pkgstate::snapshot changed = native_fixture::state_with_package(
      "other", 40, target);
  const pkgstate::state_publication_receipt stale =
      pkgstate::state_publication_receipt::stale_expected_state(
          request, changed, "pkgstate-generation/2");
  TEST_EQ(stale.outcome(),
          pkgstate::state_publication_outcome::stale_expected_state);
  TEST(!stale.resulting_snapshot());

  TEST_THROWS(pkgstate::state_error,
              pkgstate::state_publication_receipt::published(
                  request, changed, result, "pkgstate-generation/2",
                  pkgstate::state_storage_atomicity_boundary::
                      immutable_generation_selection));
}
