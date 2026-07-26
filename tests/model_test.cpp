// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "native_fixture.h"
#include "test.h"

#include <libpkgstate/error.h>

int main()
{
  using namespace pkgstate;

  TEST_EQ(package_reference("libfoo").name(), std::string("libfoo"));
  TEST_EQ(profile_reference("@toolchain").name(), std::string("@toolchain"));
  TEST_EQ(to_string(lifecycle_action::post_install),
          std::string_view("post-install"));

  TEST_THROWS(identity_error, package_reference("@wrong"));
  TEST_THROWS(identity_error, profile_reference("toolchain"));
  TEST_THROWS(identity_error, architecture_reference("x86 64"));
  TEST_THROWS(state_error,
              program(program_language::posix_shell, std::string{}));

  const requirement_origin direct(native_fixture::at("requirements.run[0]"));
  const package_requirement requirement(package_reference("libfoo"), {direct});
  TEST_EQ(requirement.origins().size(), std::size_t{1});

  TEST_THROWS(
      state_error,
      architecture_binding::make(
          {architecture_reference("aarch64")}, {},
          architecture_reference("x86_64"),
          architecture_reference("x86_64")));

  const installation_reason profile = installation_reason::profile_membership(
      profile_reference("@desktop"),
      native_fixture::identity<source_profile_identity>(4));
  TEST_EQ(profile.kind(), installation_reason_kind::profile_membership);
  TEST(profile.issuer_profile_identity().has_value());

  TEST_THROWS(state_error,
              package_metadata("summary", std::nullopt, std::nullopt,
                               {"MIT", "MIT"}));
}
