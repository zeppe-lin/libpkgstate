// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <libpkgbuild/libpkgbuild.h>
#include <libpkgimage/inspection_receipt.h>
#include <libpkgstate-source/adapter.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace build_fixture {

inline pkgsource::declaration_provenance at(const char* document,
                                            const char* path,
                                            std::uint32_t line)
{
  return pkgsource::declaration_provenance(document, path, line, 3);
}

inline pkgsource::profile_catalog profiles()
{
  using namespace pkgsource;
  return profile_catalog::seal({
      profile_declaration(
          profile_reference("@toolchain"), at("profiles.yml", "toolchain", 1),
          {
              profile_member_declaration(
                  requirement_subject(package_reference("binutils")),
                  at("profiles.yml", "toolchain[0]", 2)),
              profile_member_declaration(
                  requirement_subject(package_reference("gcc")),
                  at("profiles.yml", "toolchain[1]", 3)),
          }),
  });
}

inline pkgsource::source_snapshot source()
{
  using namespace pkgsource;
  return seal_source(
      source_origin("recipe.yml"), source_syntax::recipe_yaml_v1,
      recipe_declaration(
          package_release(package_reference("example"), "1.2.3", 1),
          package_metadata("Example", std::nullopt,
                           "https://example.invalid", {"MIT"}),
          {
              source_input::remote(
                  "https://example.invalid/example.tar.xz", "example.tar.xz",
                  digest(digest_algorithm::sha256, std::string(64, 'a'))),
              source_input::local(
                  "files/example.conf", "example.conf",
                  digest(digest_algorithm::sha256, std::string(64, 'b'))),
          },
          program(program_language::posix_shell,
                  "meson setup build\nmeson compile -C build\n"),
          {
              requirement_declaration(
                  requirement_scope::build(),
                  requirement_subject(profile_reference("@toolchain")),
                  at("recipe.yml", "requirements.build[0]", 12)),
              requirement_declaration(
                  requirement_scope::check(),
                  requirement_subject(package_reference("pkgcheck")),
                  at("recipe.yml", "requirements.check[0]", 14)),
              requirement_declaration(
                  requirement_scope::run(),
                  requirement_subject(package_reference("libfoo")),
                  at("recipe.yml", "requirements.run[0]", 16)),
          },
          {},
          architecture_requirements(
              {architecture_reference("x86_64")},
              {architecture_reference("x86_64")}),
          at("recipe.yml", "$", 1)),
      profiles());
}

inline std::vector<pkgbuild::materialized_source> materials(
    const pkgsource::source_snapshot& snapshot)
{
  std::vector<pkgbuild::materialized_source> result;
  for (const auto& input : snapshot.recipe().sources())
  {
    result.push_back(pkgbuild::materialized_source::verify(
        input, pkgbuild::sha256_digest(input.content_digest().hex())));
  }
  return result;
}

inline pkgbuild::materialized_package_input package_input(
    pkgbuild::input_scope scope, const char* name, char seed)
{
  const std::string hex(64, seed);
  return pkgbuild::materialized_package_input(
      pkgbuild::resolved_package_input::make(
          scope, pkgsource::package_reference(name),
          pkgsource::package_release(pkgsource::package_reference(name),
                                     "1.0", 1),
          pkgsource::source_snapshot_identity::from_sha256(hex),
          pkgbuild::build_result_identity::from_sha256(hex),
          pkgbuild::artifact_identity::from_sha256(hex)),
      pkgbuild::input_tree_identity::from_sha256(hex));
}

inline pkgbuild::build_request request()
{
  auto snapshot = source();
  return pkgbuild::build_request::seal(
      snapshot, materials(snapshot),
      {
          package_input(pkgbuild::input_scope::build, "binutils", 'c'),
          package_input(pkgbuild::input_scope::build, "gcc", 'd'),
          package_input(pkgbuild::input_scope::check, "pkgcheck", 'e'),
      },
      pkgsource::architecture_reference("x86_64"),
      pkgsource::architecture_reference("x86_64"),
      pkgbuild::build_policy::make(
          pkgbuild::environment_policy::hermetic(4, 0022, 1700000000)));
}

inline pkgbuild::payload_manifest payload(std::uint32_t regular_mode = 0755)
{
  using namespace pkgbuild;
  return payload_manifest::seal({
      payload_entry::directory(payload_path::parse("usr/bin"), 0755, 0, 0,
                               payload_time{1700000000, 0}),
      payload_entry::regular(payload_path::parse("usr/bin/example"),
                             regular_mode, 0, 0, 3,
                             payload_time{1700000000, 1},
                             sha256_digest(std::string(64, 'f'))),
      payload_entry::hardlink(payload_path::parse("usr/bin/example-link"),
                              regular_mode, 0, 0,
                              payload_time{1700000000, 1},
                              payload_path::parse("usr/bin/example")),
      payload_entry::symlink(payload_path::parse("usr/bin/example-symlink"),
                             0777, 0, 0,
                             payload_time{1700000000, 0}, "example"),
  });
}

inline pkgbuild::sealed_artifact artifact(char seed = '1')
{
  return pkgbuild::sealed_artifact::make(
      pkgbuild::artifact_encoding::package_tar_v1,
      pkgbuild::artifact_compression::zstd, 4096,
      pkgbuild::sha256_digest(std::string(64, seed)));
}

inline pkgbuild::build_result result()
{
  return pkgbuild::build_result::succeeded(
      request(), payload(), artifact(),
      pkgbuild::execution_evidence_identity::from_sha256(std::string(64, '9')));
}

inline pkgimage::package_image image(std::uint32_t regular_mode = 0755)
{
  using namespace pkgimage;
  std::vector<package_entry> entries;

  package_entry directory(package_path::parse("usr/bin"),
                          entry_type::directory);
  directory.mode = 0755;
  directory.mtime = 1700000000;
  entries.push_back(std::move(directory));

  package_entry regular(package_path::parse("usr/bin/example"),
                        entry_type::regular);
  regular.mode = regular_mode;
  regular.size = 3;
  regular.mtime = 1700000000;
  regular.mtime_nanoseconds = 1;
  regular.regular_content = regular_content_digest::parse(
      "v1:sha256:" + std::string(64, 'f'));
  entries.push_back(std::move(regular));

  package_entry hardlink(package_path::parse("usr/bin/example-link"),
                         entry_type::hardlink);
  hardlink.mode = regular_mode;
  hardlink.mtime = 1700000000;
  hardlink.mtime_nanoseconds = 1;
  hardlink.hardlink_target = package_path::parse("usr/bin/example");
  entries.push_back(std::move(hardlink));

  package_entry symlink(package_path::parse("usr/bin/example-symlink"),
                        entry_type::symlink);
  symlink.mode = 0777;
  symlink.mtime = 1700000000;
  symlink.symlink_target = "example";
  entries.push_back(std::move(symlink));

  return package_image(std::move(entries));
}

inline pkgimage::inspected_package_image inspected(
    char archive_seed = '1', std::uint32_t regular_mode = 0755)
{
  pkgimage::package_image value = image(regular_mode);
  pkgimage::archive_inspection_receipt receipt(
      pkgimage::archive_backend_identity::parse("test/archive-v1"),
      pkgimage::complete_archive_digest::parse(
          "v1:sha256:" + std::string(64, archive_seed)),
      value.identity(), value.size());
  return pkgimage::inspected_package_image(
      std::move(value), std::move(receipt));
}

inline pkgstate::package_source_record state_source()
{
  const auto snapshot = source();
  return pkgstate::source_adapter::project_source(
      snapshot, pkgsource::architecture_reference("x86_64"),
      pkgsource::architecture_reference("x86_64"));
}

} // namespace build_fixture
