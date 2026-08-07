// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "native_fixture.h"
#include "test.h"

#include <libpkgstate/error.h>
#include <libpkgstate/publication_projection.h>

int
main()
{
  using namespace pkgstate;

  const state_target_binding binding = native_fixture::target();
  const snapshot empty = snapshot::make(binding);
  const installed_package first = native_fixture::package("first", 20, binding);
  const auto install = state_publication_request::make(
      empty,
      {package_state_delta::install(
          first, first.receipt().operation_plan(),
          first.receipt().application_evidence())});
  const snapshot installed = project_publication_request(install, empty);
  TEST_EQ(installed.packages().size(), std::size_t{1});
  TEST_EQ(installed.packages().front().identity(), first.identity());

  const installed_package replacement =
      native_fixture::package("first", 60, binding);
  const auto replace = state_publication_request::make(
      installed,
      {package_state_delta::replace(
          first.identity(), replacement, replacement.receipt().operation_plan(),
          replacement.receipt().application_evidence())});
  const snapshot replaced = project_publication_request(replace, installed);
  TEST_EQ(replaced.packages().size(), std::size_t{1});
  TEST_EQ(replaced.packages().front().identity(), replacement.identity());

  const auto remove = state_publication_request::make(
      replaced,
      {package_state_delta::remove(
          replacement.release().name(), replacement.identity(),
          replacement.receipt().operation_plan(),
          replacement.receipt().application_evidence())});
  const snapshot removed = project_publication_request(remove, replaced);
  TEST(removed.packages().empty());

  TEST_THROWS(state_error, project_publication_request(install, installed));
  TEST_THROWS(
      state_error,
      project_publication_request(
          install, snapshot::make(native_fixture::target(90))));
}
