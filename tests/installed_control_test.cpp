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

  TEST_EQ(explicit_control.build().source_record(),
          explicit_control.source().identity());
  TEST_EQ(explicit_control.build().request(),
          native_fixture::identity<pkgstate::build_request_identity>(24));
  TEST_EQ(explicit_control.build().build_inputs(),
          native_fixture::identity<pkgstate::build_input_set_identity>(26));
  TEST_EQ(explicit_control.build().environment_policy(),
          native_fixture::identity<pkgstate::environment_policy_identity>(27));
  TEST_EQ(explicit_control.build().build_policy(),
          native_fixture::identity<pkgstate::build_policy_identity>(28));
  TEST_EQ(explicit_control.build().build_result(),
          native_fixture::identity<pkgstate::build_result_identity>(29));
  TEST_EQ(explicit_control.build().payload_manifest(),
          native_fixture::identity<pkgstate::payload_manifest_identity>(30));
  TEST_EQ(explicit_control.build().artifact(),
          native_fixture::identity<pkgstate::build_artifact_identity>(31));
  TEST_EQ(explicit_control.build().artifact_content(),
          native_fixture::identity<pkgstate::artifact_content_identity>(32));
  TEST_EQ(explicit_control.build().artifact_binding(),
          native_fixture::identity<pkgstate::artifact_binding_identity>(33));
  TEST_EQ(explicit_control.build().execution_evidence(),
          native_fixture::identity<pkgstate::execution_evidence_identity>(34));
  TEST_EQ(explicit_control.build().build_image(),
          native_fixture::identity<pkgstate::build_image_identity>(35));
  TEST_EQ(explicit_control.build().artifact_image(),
          native_fixture::identity<pkgstate::artifact_image_identity>(36));
  TEST_EQ(explicit_control.build().artifact_inspection(),
          native_fixture::identity<pkgstate::artifact_inspection_identity>(37));

  pkgstate::package_source_record mismatched_source =
      native_fixture::source("other", 70);
  TEST_THROWS(
      pkgstate::state_error,
      pkgstate::installed_control::make(
          mismatched_source,
          pkgstate::installation_reason::explicit_request(),
          explicit_control.build()));
}
