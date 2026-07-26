// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "native_fixture.h"
#include "test.h"

#include <string>

#include <libpkgstate/installed_control.h>

int main()
{
  const pkgstate::installed_control explicit_control =
      native_fixture::control();
  TEST_EQ(explicit_control.reason().kind(),
          pkgstate::installation_reason_kind::explicit_request);
  TEST(!explicit_control.reason().issuer_package());
  TEST_EQ(explicit_control.source().runtime_requirements().front()
              .package().name(),
          "libfoo");

  const pkgstate::installed_control dependency_control =
      native_fixture::control(
          "example", 20,
          pkgstate::installation_reason::runtime_dependency(
              pkgstate::package_reference("consumer")));
  TEST_NE(dependency_control.identity(), explicit_control.identity());
  TEST_EQ(dependency_control.reason().issuer_package()->name(), "consumer");

  const pkgstate::installed_control profile_control =
      native_fixture::control(
          "example", 20,
          pkgstate::installation_reason::profile_membership(
              pkgstate::profile_reference("@desktop"),
              native_fixture::identity<pkgstate::source_profile_identity>(90)));
  TEST_EQ(profile_control.reason().kind(),
          pkgstate::installation_reason_kind::profile_membership);
  TEST_EQ(profile_control.reason().issuer_profile()->name(), "@desktop");

  const pkgstate::installation_reason policy =
      pkgstate::installation_reason::system_policy("base-system");
  TEST_EQ(policy.kind(), pkgstate::installation_reason_kind::system_policy);
  TEST_EQ(*policy.policy(), "base-system");
  TEST_THROWS(pkgstate::state_error,
              pkgstate::installation_reason::system_policy("bad\npolicy"));

  TEST_EQ(explicit_control.build().candidate_control(),
          native_fixture::identity<pkgstate::candidate_control_identity>(24));
  TEST_EQ(explicit_control.build().build_inputs(),
          native_fixture::identity<pkgstate::build_input_set_identity>(25));
  TEST_EQ(explicit_control.build().build_result(),
          native_fixture::identity<pkgstate::build_result_identity>(26));
}
