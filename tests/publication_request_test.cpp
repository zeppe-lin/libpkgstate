// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "native_fixture.h"
#include "test.h"

#include <libpkgstate/error.h>

int main()
{
  using namespace pkgstate;
  const state_target_binding binding = native_fixture::target();
  const snapshot empty = snapshot::make(binding);
  installed_package proposed = native_fixture::package("example", 20, binding);

  package_state_delta install = package_state_delta::install(
      proposed, proposed.receipt().operation_plan(),
      proposed.receipt().application_evidence());
  const state_publication_request request =
      state_publication_request::make(empty, {install});
  TEST_EQ(request.schema_version(), state_publication_request_schema_version);
  TEST_EQ(request.deltas().size(), std::size_t{1});

  TEST_THROWS(
      state_error,
      state_publication_request::make(
          empty,
          {package_state_delta::install(
              proposed,
              native_fixture::identity<operation_plan_identity>(99),
              proposed.receipt().application_evidence())}));

  const snapshot installed = snapshot::make(binding, {proposed});
  TEST_THROWS(state_error,
              state_publication_request::make(installed, {install}));
}
