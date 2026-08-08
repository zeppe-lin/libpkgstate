// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/state.h"
#include "../support/test.h"

#include <libpkgstate/error.h>

int main()
{
  using namespace pkgstate;
  const state_target_binding binding = state_fixture::target();
  const snapshot empty = snapshot::make(binding);
  const installed_package first = state_fixture::package("first", 20, binding);

  const package_state_delta install = state_fixture::install_delta(first);
  TEST_EQ(install.kind(), package_state_delta_kind::install);
  TEST_EQ(install.package_name(), std::string("first"));
  TEST(!install.expected_package());
  TEST_EQ(install.proposed_package()->identity(), first.identity());

  const state_publication_request install_request =
      state_publication_request::make(empty, {install});
  TEST_EQ(install_request.schema_version(), state_publication_request_schema_version);
  TEST_EQ(install_request.expected_snapshot(), empty.identity());
  TEST_EQ(install_request.target_binding(), binding);
  TEST_EQ(install_request.deltas().size(), std::size_t{1});
  TEST(!install_request.transaction_evidence());

  TEST_THROWS(state_error, state_publication_request::make(empty, {}));
  TEST_THROWS(
      state_error,
      package_state_delta::install(
          first, state_fixture::identity<operation_plan_identity>(99),
          first.receipt().application_evidence()));

  const snapshot installed = snapshot::make(binding, {first});
  TEST_THROWS(state_error,
              state_publication_request::make(installed, {install}));

  const installed_package replacement = state_fixture::package("first", 40, binding);
  const package_state_delta replace = package_state_delta::replace(
      first.identity(), replacement, replacement.receipt().operation_plan(),
      replacement.receipt().application_evidence());
  const state_publication_request replace_request =
      state_publication_request::make(installed, {replace});
  TEST_EQ(replace_request.deltas().front().kind(), package_state_delta_kind::replace);
  TEST_EQ(*replace_request.deltas().front().expected_package(), first.identity());
  TEST_THROWS(state_error,
              package_state_delta::replace(
                  first.identity(), first, first.receipt().operation_plan(),
                  first.receipt().application_evidence()));
  TEST_THROWS(state_error,
              state_publication_request::make(
                  installed,
                  {package_state_delta::replace(
                      state_fixture::identity<installed_package_identity>(98),
                      replacement, replacement.receipt().operation_plan(),
                      replacement.receipt().application_evidence())}));

  const package_state_delta remove = package_state_delta::remove(
      "first", first.identity(), first.receipt().operation_plan(),
      first.receipt().application_evidence());
  const state_publication_request remove_request =
      state_publication_request::make(installed, {remove});
  TEST_EQ(remove_request.deltas().front().kind(), package_state_delta_kind::remove);
  TEST(!remove_request.deltas().front().proposed_package());
  TEST_THROWS(state_error,
              state_publication_request::make(
                  empty,
                  {package_state_delta::remove(
                      "first", first.identity(), first.receipt().operation_plan(),
                      first.receipt().application_evidence())}));
  TEST_THROWS(state_error,
              package_state_delta::remove(
                  "bad\nname", first.identity(), first.receipt().operation_plan(),
                  first.receipt().application_evidence()));

  const transaction_evidence_identity transaction =
      state_fixture::identity<transaction_evidence_identity>(120);
  const installed_package second = state_fixture::package(
      "second", 60, installed.target_binding(), transaction);
  const package_state_delta second_install = state_fixture::install_delta(second);
  TEST_THROWS(state_error,
              state_publication_request::make(
                  installed, {remove, second_install}));
  const state_publication_request composed = state_publication_request::make(
      installed, {second_install, remove}, transaction);
  TEST_EQ(composed.deltas().size(), std::size_t{2});
  TEST_EQ(composed.deltas()[0].package_name(), std::string("first"));
  TEST_EQ(composed.deltas()[1].package_name(), std::string("second"));
  TEST_EQ(*composed.transaction_evidence(), transaction);

  TEST_THROWS(state_error,
              state_publication_request::make(
                  installed, {remove, second_install},
                  state_fixture::identity<transaction_evidence_identity>(121)));
  TEST_THROWS(state_error,
              state_publication_request::make(
                  installed, {remove, remove}, transaction));

  const installed_package foreign =
      state_fixture::package("foreign", 80, state_fixture::target(150));
  TEST_THROWS(state_error,
              state_publication_request::make(
                  empty, {state_fixture::install_delta(foreign)}));
}
