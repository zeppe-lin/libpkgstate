// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/state.h"
#include "../support/test.h"

#include <libpkgstate/error.h>
#include <libpkgstate/model.h>

#include <optional>
#include <string>

int main()
{
  using namespace pkgstate;
  TEST_EQ(to_string(lifecycle_action::pre_install), std::string_view("pre-install"));
  TEST_EQ(to_string(lifecycle_action::post_install), std::string_view("post-install"));
  TEST_EQ(to_string(lifecycle_action::pre_remove), std::string_view("pre-remove"));
  TEST_EQ(to_string(lifecycle_action::post_remove), std::string_view("post-remove"));
  TEST_EQ(to_string(static_cast<lifecycle_action>(0)), std::string_view("unknown"));
  TEST_EQ(to_string(program_language::posix_shell), std::string_view("posix-shell"));
  TEST_EQ(to_string(static_cast<program_language>(0)), std::string_view("unknown"));

  const program script(program_language::posix_shell, "echo ok\n");
  TEST_EQ(script.material(), std::string("echo ok\n"));
  TEST_THROWS(state_error, program(program_language::posix_shell, std::string{}));
  TEST_THROWS(state_error, program(static_cast<program_language>(0), "echo x\n"));
  TEST_THROWS(state_error,
              program(program_language::posix_shell, std::string("bad\0body", 8)));

  const requirement_origin direct(state_fixture::at("requirements.run[0]"));
  const lifecycle_program lifecycle(lifecycle_action::post_install, script);
  TEST_EQ(lifecycle.action(), lifecycle_action::post_install);
  TEST_THROWS(state_error,
              lifecycle_program(static_cast<lifecycle_action>(0), script));
  const lifecycle_requirement lifecycle_requirement_value(
      lifecycle_action::pre_remove,
      package_requirement(package_reference("libfoo"), {direct}));
  TEST_EQ(lifecycle_requirement_value.action(), lifecycle_action::pre_remove);
  TEST_THROWS(state_error,
              lifecycle_requirement(
                  static_cast<lifecycle_action>(0),
                  package_requirement(package_reference("libfoo"), {direct})));

  const package_metadata metadata(
      "summary", std::optional<std::string>("long\ntext\tvalue"),
      std::optional<std::string>("https://example.invalid"),
      {"MIT", "GPL-3.0-or-later"});
  TEST_EQ(metadata.licenses().front(), std::string("GPL-3.0-or-later"));
  TEST_EQ(metadata.licenses().back(), std::string("MIT"));
  TEST_THROWS(state_error, package_metadata("", std::nullopt, std::nullopt, {"MIT"}));
  TEST_THROWS(state_error, package_metadata("summary", std::nullopt, std::nullopt, {}));
  TEST_THROWS(state_error,
              package_metadata("summary", std::nullopt, std::nullopt, {"MIT", "MIT"}));
  TEST_THROWS(state_error,
              package_metadata("summary\n", std::nullopt, std::nullopt, {"MIT"}));

  const selected_profile selected(
      profile_reference("@desktop"),
      state_fixture::identity<source_profile_identity>(4),
      {state_fixture::at("requirements.build[1]", 9),
       state_fixture::at("requirements.build[0]", 8)});
  TEST_EQ(selected.declarations().size(), std::size_t{2});
  TEST(selected.declarations()[0] < selected.declarations()[1]);
  TEST_THROWS(state_error,
              selected_profile(profile_reference("@desktop"),
                               state_fixture::identity<source_profile_identity>(4), {}));
  TEST_THROWS(state_error,
              selected_profile(
                  profile_reference("@desktop"),
                  state_fixture::identity<source_profile_identity>(4),
                  {state_fixture::at("x"), state_fixture::at("x")}));

  const architecture_binding open = architecture_binding::make(
      {}, {}, architecture_reference("aarch64"), architecture_reference("riscv64"));
  TEST(open.declared_build().empty());
  TEST_EQ(open.selected_target().name(), std::string("riscv64"));
  const architecture_binding closed = architecture_binding::make(
      {architecture_reference("x86_64"), architecture_reference("aarch64")},
      {architecture_reference("riscv64"), architecture_reference("x86_64")},
      architecture_reference("aarch64"), architecture_reference("riscv64"));
  TEST_EQ(closed.declared_build().front().name(), std::string("aarch64"));
  TEST_THROWS(state_error,
              architecture_binding::make(
                  {architecture_reference("aarch64")}, {},
                  architecture_reference("x86_64"), architecture_reference("x86_64")));
  TEST_THROWS(state_error,
              architecture_binding::make(
                  {}, {architecture_reference("aarch64")},
                  architecture_reference("x86_64"), architecture_reference("x86_64")));
  TEST_THROWS(state_error,
              architecture_binding::make(
                  {architecture_reference("x86_64"), architecture_reference("x86_64")}, {},
                  architecture_reference("x86_64"), architecture_reference("x86_64")));
}
