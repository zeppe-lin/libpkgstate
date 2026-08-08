// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/state.h"
#include "../support/test.h"

#include <libpkgstate/error.h>
#include <libpkgstate/model.h>

int main()
{
  using namespace pkgstate;
  const installation_reason explicit_reason = installation_reason::explicit_request();
  TEST_EQ(explicit_reason.kind(), installation_reason_kind::explicit_request);
  TEST(!explicit_reason.issuer_package());
  TEST(!explicit_reason.issuer_profile());
  TEST(!explicit_reason.policy());

  const installation_reason dependency_reason =
      installation_reason::runtime_dependency(package_reference("consumer"));
  TEST_EQ(dependency_reason.kind(), installation_reason_kind::runtime_dependency);
  TEST_EQ(dependency_reason.issuer_package()->name(), std::string("consumer"));

  const installation_reason profile_reason = installation_reason::profile_membership(
      profile_reference("@desktop"), state_fixture::identity<source_profile_identity>(5));
  TEST_EQ(profile_reason.kind(), installation_reason_kind::profile_membership);
  TEST_EQ(profile_reason.issuer_profile()->name(), std::string("@desktop"));
  TEST(profile_reason.issuer_profile_identity().has_value());

  const installation_reason policy_reason = installation_reason::system_policy("base-system");
  TEST_EQ(policy_reason.kind(), installation_reason_kind::system_policy);
  TEST_EQ(*policy_reason.policy(), std::string("base-system"));
  TEST_THROWS(state_error, installation_reason::system_policy(""));
  TEST_THROWS(state_error, installation_reason::system_policy("bad\npolicy"));
}
