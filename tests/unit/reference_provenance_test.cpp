// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/state.h"
#include "../support/test.h"

#include <libpkgstate/error.h>
#include <libpkgstate/model.h>

int main()
{
  using namespace pkgstate;
  TEST_EQ(package_reference("libfoo").name(), std::string("libfoo"));
  TEST_EQ(package_reference("libfoo+ssl_2.0").name(), std::string("libfoo+ssl_2.0"));
  TEST_EQ(profile_reference("@toolchain").name(), std::string("@toolchain"));
  TEST_EQ(architecture_reference("x86_64").name(), std::string("x86_64"));
  TEST_THROWS(identity_error, package_reference("@wrong"));
  TEST_THROWS(identity_error, package_reference("Upper"));
  TEST_THROWS(identity_error, profile_reference("toolchain"));
  TEST_THROWS(identity_error, profile_reference("@"));
  TEST_THROWS(identity_error, architecture_reference("x86 64"));

  const declaration_provenance declaration("recipe.yml", "requirements.run[0]", 12, 9);
  TEST_EQ(declaration.document(), std::string("recipe.yml"));
  TEST_EQ(declaration.path(), std::string("requirements.run[0]"));
  TEST_EQ(declaration.line(), std::uint32_t{12});
  TEST_EQ(declaration.column(), std::uint32_t{9});
  TEST_THROWS(state_error, declaration_provenance("", "x", 1, 1));
  TEST_THROWS(state_error, declaration_provenance("recipe.yml", "x\ny", 1, 1));
  TEST_THROWS(state_error, declaration_provenance("recipe.yml", "x", 0, 1));
  TEST_THROWS(state_error, declaration_provenance("recipe.yml", "x", 1, 0));

  const profile_expansion_step profile_step(
      profile_reference("@toolchain"), requirement_member_kind::profile,
      "@compiler", state_fixture::at("profiles.toolchain[0]", 4));
  const profile_expansion_step package_step(
      profile_reference("@compiler"), requirement_member_kind::package,
      "libfoo", state_fixture::at("profiles.compiler[0]", 7));
  TEST_EQ(profile_step.member(), std::string("@compiler"));
  TEST_EQ(package_step.member(), std::string("libfoo"));
  TEST_THROWS(identity_error,
              profile_expansion_step(
                  profile_reference("@toolchain"), requirement_member_kind::package,
                  "@bad", state_fixture::at("x")));
  TEST_THROWS(state_error,
              profile_expansion_step(
                  profile_reference("@toolchain"),
                  static_cast<requirement_member_kind>(0), "libfoo",
                  state_fixture::at("x")));

  const requirement_origin direct(state_fixture::at("requirements.run[0]"));
  const requirement_origin expanded(
      state_fixture::at("requirements.run[1]"), {profile_step, package_step});
  const package_requirement requirement(package_reference("libfoo"), {expanded, direct});
  TEST_EQ(requirement.origins().size(), std::size_t{2});
  TEST(requirement.origins().front() < requirement.origins().back());
  TEST_THROWS(state_error, package_requirement(package_reference("libfoo"), {}));
  TEST_THROWS(state_error,
              package_requirement(package_reference("libfoo"), {direct, direct}));
  TEST_THROWS(state_error,
              package_requirement(
                  package_reference("libbar"),
                  {requirement_origin(state_fixture::at("requirements.run[2]"),
                                      {profile_step, package_step})}));
}
