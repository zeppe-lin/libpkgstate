// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "test.h"

#include <cstdint>
#include <string>

#include <libpkgsource/libpkgsource.h>
#include <libpkgstate-source/adapter.h>

namespace {

pkgsource::declaration_provenance at(const char* document,
                                     const char* path,
                                     std::uint32_t line)
{
  return pkgsource::declaration_provenance(document, path, line, 3);
}

pkgsource::source_snapshot source_snapshot()
{
  const pkgsource::profile_catalog profiles = pkgsource::profile_catalog::seal({
      pkgsource::profile_declaration(
          pkgsource::profile_reference("@compiler"),
          at("profiles.yml", "compiler", 1),
          {pkgsource::profile_member_declaration(
              pkgsource::requirement_subject(
                  pkgsource::package_reference("gcc")),
              at("profiles.yml", "compiler[0]", 2))}),
      pkgsource::profile_declaration(
          pkgsource::profile_reference("@toolchain"),
          at("profiles.yml", "toolchain", 4),
          {
              pkgsource::profile_member_declaration(
                  pkgsource::requirement_subject(
                      pkgsource::package_reference("binutils")),
                  at("profiles.yml", "toolchain[0]", 5)),
              pkgsource::profile_member_declaration(
                  pkgsource::requirement_subject(
                      pkgsource::profile_reference("@compiler")),
                  at("profiles.yml", "toolchain[1]", 6)),
          }),
  });

  return pkgsource::seal_source(
      pkgsource::source_origin("recipe.yml"),
      pkgsource::source_syntax::recipe_yaml_v1,
      pkgsource::recipe_declaration(
          pkgsource::package_release(
              pkgsource::package_reference("example"), "1.2.3", 2),
          pkgsource::package_metadata(
              "Example package", "Long description",
              "https://example.invalid", {"MIT"}),
          {},
          pkgsource::program(
              pkgsource::program_language::posix_shell,
              "echo build\n"),
          {
              pkgsource::requirement_declaration(
                  pkgsource::requirement_scope::build(),
                  pkgsource::requirement_subject(
                      pkgsource::profile_reference("@toolchain")),
                  at("recipe.yml", "requirements.build[0]", 10)),
              pkgsource::requirement_declaration(
                  pkgsource::requirement_scope::run(),
                  pkgsource::requirement_subject(
                      pkgsource::package_reference("libfoo")),
                  at("recipe.yml", "requirements.run[0]", 12)),
              pkgsource::requirement_declaration(
                  pkgsource::requirement_scope::lifecycle(
                      pkgsource::lifecycle_action::post_install),
                  pkgsource::requirement_subject(
                      pkgsource::package_reference("desktop-file-utils")),
                  at("recipe.yml",
                     "requirements.lifecycle.post-install[0]", 14)),
          },
          {
              pkgsource::lifecycle_program(
                  pkgsource::lifecycle_action::pre_install,
                  pkgsource::program(
                      pkgsource::program_language::posix_shell,
                      "echo pre-install\n")),
              pkgsource::lifecycle_program(
                  pkgsource::lifecycle_action::post_install,
                  pkgsource::program(
                      pkgsource::program_language::posix_shell,
                      "echo post-install\n")),
              pkgsource::lifecycle_program(
                  pkgsource::lifecycle_action::pre_remove,
                  pkgsource::program(
                      pkgsource::program_language::posix_shell,
                      "echo pre-remove\n")),
              pkgsource::lifecycle_program(
                  pkgsource::lifecycle_action::post_remove,
                  pkgsource::program(
                      pkgsource::program_language::posix_shell,
                      "echo post-remove\n")),
          },
          pkgsource::architecture_requirements(
              {pkgsource::architecture_reference("x86_64")},
              {pkgsource::architecture_reference("x86_64")}),
          at("recipe.yml", "$", 1)),
      profiles);
}

void projects_complete_source_authority()
{
  const pkgsource::source_snapshot source = source_snapshot();
  const pkgstate::package_source_record record =
      pkgstate::source_adapter::project_source(
          source,
          pkgsource::architecture_reference("x86_64"),
          pkgsource::architecture_reference("x86_64"));

  TEST_EQ(record.release().identity().string(),
          std::string("v1:sha256:") + source.recipe().release().identity().hex());
  TEST_EQ(record.release().package().name(), "example");
  TEST_EQ(record.release().version(), "1.2.3");
  TEST_EQ(record.release().release(), std::uint32_t{2});
  TEST_EQ(record.metadata().summary(), "Example package");
  TEST_EQ(record.runtime_requirements().size(), std::size_t{1});
  TEST_EQ(record.runtime_requirements().front().package().name(), "libfoo");
  TEST_EQ(record.lifecycle_programs().size(), std::size_t{4});
  TEST(record.lifecycle(pkgstate::lifecycle_action::pre_install) != nullptr);
  TEST(record.lifecycle(pkgstate::lifecycle_action::post_remove) != nullptr);
  TEST_EQ(record.lifecycle_requirements().size(), std::size_t{1});
  TEST_EQ(record.lifecycle_requirements().front().action(),
          pkgstate::lifecycle_action::post_install);
  TEST_EQ(record.lifecycle_requirements().front()
              .requirement().package().name(),
          "desktop-file-utils");
  TEST_EQ(record.selected_profiles().size(), std::size_t{1});
  TEST_EQ(record.selected_profiles().front().profile().name(), "@toolchain");
  TEST_EQ(record.architectures().selected_build().name(), "x86_64");
  TEST_EQ(record.architectures().selected_target().name(), "x86_64");
  TEST_EQ(record.recipe().string(),
          std::string("v1:sha256:") + source.recipe().identity().hex());
  TEST_EQ(record.snapshot().string(),
          std::string("v1:sha256:") + source.identity().hex());
}

void rejects_unaccepted_architecture()
{
  const pkgsource::source_snapshot source = source_snapshot();
  try
  {
    (void)pkgstate::source_adapter::project_source(
        source,
        pkgsource::architecture_reference("aarch64"),
        pkgsource::architecture_reference("x86_64"));
    TEST(false);
  }
  catch (const pkgstate::source_adapter::projection_error& error)
  {
    TEST_EQ(error.code(),
            pkgstate::source_adapter::projection_error_code::
                architecture_selection);
  }
}

} // namespace

int main()
{
  projects_complete_source_authority();
  rejects_unaccepted_architecture();
}
