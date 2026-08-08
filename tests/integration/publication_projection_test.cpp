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
  const snapshot empty = snapshot::make(binding);
  const installed_package first = state_fixture::package("first", 20, binding);

  const state_publication_request install = state_publication_request::make(
      empty, {state_fixture::install_delta(first)});
  const snapshot installed = project_publication_request(install, empty);
  TEST_EQ(installed.size(), std::size_t{1});
  TEST_EQ(installed.find_package("first")->identity(), first.identity());

  const installed_package replacement = state_fixture::package("first", 40, binding);
  const state_publication_request replace = state_publication_request::make(
      installed,
      {package_state_delta::replace(
          first.identity(), replacement, replacement.receipt().operation_plan(),
          replacement.receipt().application_evidence())});
  const snapshot replaced = project_publication_request(replace, installed);
  TEST_EQ(replaced.find_package("first")->identity(), replacement.identity());

  const state_publication_request remove = state_publication_request::make(
      replaced,
      {package_state_delta::remove(
          "first", replacement.identity(), replacement.receipt().operation_plan(),
          replacement.receipt().application_evidence())});
  const snapshot removed = project_publication_request(remove, replaced);
  TEST(removed.packages().empty());

  const transaction_evidence_identity transaction =
      state_fixture::identity<transaction_evidence_identity>(100);
  const installed_package alpha = state_fixture::package("alpha", 60, binding);
  const snapshot alpha_state = snapshot::make(binding, {alpha});
  const installed_package beta = state_fixture::package("beta", 80, binding, transaction);
  const state_publication_request composed = state_publication_request::make(
      alpha_state,
      {package_state_delta::remove(
           "alpha", alpha.identity(), alpha.receipt().operation_plan(),
           alpha.receipt().application_evidence()),
       state_fixture::install_delta(beta)},
      transaction);
  const snapshot composed_result = project_publication_request(composed, alpha_state);
  TEST(composed_result.find_package("alpha") == nullptr);
  TEST_EQ(composed_result.find_package("beta")->identity(), beta.identity());

  TEST_THROWS(state_error, project_publication_request(install, installed));
  TEST_THROWS(state_error,
              project_publication_request(
                  install, snapshot::make(state_fixture::target(90))));
}
