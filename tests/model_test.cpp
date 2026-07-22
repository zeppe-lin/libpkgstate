// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/libpkgstate.h>

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "test.h"

namespace {

pkgstate::package_path
path(const char* value)
{
  return pkgstate::package_path::parse(value);
}

pkgstate::owned_entry
file(const char* value)
{
  return {path(value), pkgstate::owned_entry_type::non_directory};
}

pkgstate::owned_entry
directory(const char* value)
{
  return {path(value), pkgstate::owned_entry_type::directory};
}

template<typename Identity>
Identity
identity(char digit)
{
  return Identity::parse("v1:sha256:" + std::string(64, digit));
}

pkgstate::state_target_binding
binding(char first)
{
  return pkgstate::state_target_binding::make(
      identity<pkgstate::managed_target_identity>(first),
      identity<pkgstate::state_store_identity>(
          static_cast<char>(first + 1)),
      identity<pkgstate::root_view_identity>(
          static_cast<char>(first + 2)),
      identity<pkgstate::state_backend_identity>(
          static_cast<char>(first + 3)),
      identity<pkgstate::publication_domain_identity>(
          static_cast<char>(first + 4)));
}

pkgstate::installed_package
package(const char* name,
        const char* version,
        const char* distribution_release,
        const pkgstate::state_target_binding& target,
        std::vector<pkgstate::owned_entry> manifest)
{
  const pkgstate::package_release release =
      pkgstate::package_release::make(name, version, distribution_release);
  const pkgstate::installed_control control =
      pkgstate::installed_control::make(release, {}, {}, {}, {}, {});
  return pkgstate::installed_package::make(
      release, control, target, std::move(manifest));
}

} // namespace

int
main()
{
  static_assert(!std::is_default_constructible_v<pkgstate::installed_package>);
  static_assert(!std::is_constructible_v<
                pkgstate::installed_package,
                pkgstate::installed_package_identity,
                pkgstate::package_release,
                pkgstate::installed_control,
                pkgstate::state_target_binding,
                std::vector<pkgstate::owned_entry>>);
  static_assert(!std::is_default_constructible_v<pkgstate::snapshot>);

  const pkgstate::state_target_binding target = binding('1');
  const pkgstate::installed_package base = package(
      "base", "1.0", "1", target,
      {file("usr/bin/base"), directory("etc/base.conf")});

  CHECK(base.release() == pkgstate::package_release::make("base", "1.0", "1"));
  CHECK(base.control().release() == base.release());
  CHECK(base.target_binding() == target);
  CHECK(base.size() == 2);
  CHECK(base.manifest()[0].path.string() == "etc/base.conf");
  CHECK(base.manifest()[1].path.string() == "usr/bin/base");
  CHECK(base.owns(path("usr/bin/base")));
  CHECK(base.find(path("etc/base.conf"))->type ==
        pkgstate::owned_entry_type::directory);
  CHECK(!base.owns(path("usr/bin/other")));

  CHECK(base.identity().string() ==
        "v1:sha256:"
        "f5ee24bdbbd156163ae70a0984a755c434cee1f4e82e1f6127f799288405ae61");

  const pkgstate::installed_package permuted = package(
      "base", "1.0", "1", target,
      {directory("etc/base.conf"), file("usr/bin/base")});
  CHECK(base == permuted);
  CHECK(!(base != permuted));

  const pkgstate::installed_package another_target = package(
      "base", "1.0", "1", binding('5'),
      {directory("etc/base.conf"), file("usr/bin/base")});
  CHECK(base.identity() != another_target.identity());

  const pkgstate::package_release base_release = base.release();
  const pkgstate::installed_control with_dependency =
      pkgstate::installed_control::make(
          base_release, {},
          {pkgstate::runtime_dependency_declaration::make("libc")},
          {}, {}, {});
  const pkgstate::installed_package another_control =
      pkgstate::installed_package::make(
          base_release, with_dependency, target,
          {directory("etc/base.conf"), file("usr/bin/base")});
  CHECK(base.identity() != another_control.identity());

  const pkgstate::installed_package empty =
      package("empty", "1", "1", target, {});
  CHECK(empty.size() == 0);

  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(package(
        "duplicate", "1", "1", target,
        {file("usr/bin/tool"), directory("usr/bin/tool")}));
  });

  check_throws<pkgstate::state_error>([&] {
    const pkgstate::package_release release =
        pkgstate::package_release::make("base", "2", "1");
    static_cast<void>(pkgstate::installed_package::make(
        release, base.control(), target, {}));
  });

  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::installed_package::make(
        base.release(), base.control(), target,
        {{path("usr/bin/base"),
          static_cast<pkgstate::owned_entry_type>(99)}}));
  });

  const pkgstate::snapshot empty_state =
      pkgstate::snapshot::make(target);
  CHECK(empty_state.size() == 0);
  CHECK(empty_state.target_binding() == target);

  const pkgstate::snapshot state = pkgstate::snapshot::make(target, {
      package("zlib", "1.3", "1", target, {file("usr/lib/libz.so")}),
      package("base", "1.0", "1", target,
              {file("usr/bin/base"), directory("usr/share/common")}),
      package("other", "2", "1", target,
              {directory("usr/share/common")}),
  });

  CHECK(state.schema_version() == pkgstate::installed_state_schema_version);
  CHECK(state.target_binding() == target);
  CHECK(state.size() == 3);
  CHECK(state.packages()[0].release().name() == "base");
  CHECK(state.packages()[1].release().name() == "other");
  CHECK(state.packages()[2].release().name() == "zlib");
  CHECK(state.find_package("other") != nullptr);
  CHECK(state.find_package("missing") == nullptr);
  CHECK(state.is_owned(path("usr/share/common")));
  CHECK(!state.is_owned(path("usr/share/absent")));

  const auto owners = state.owners(path("usr/share/common"));
  CHECK(owners.size() == 2);
  CHECK(owners[0]->release().name() == "base");
  CHECK(owners[1]->release().name() == "other");

  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::snapshot::make(target, {
        package("same", "1", "1", target, {}),
        package("same", "2", "1", target, {}),
    }));
  });

  check_throws<pkgstate::state_error>([&] {
    static_cast<void>(pkgstate::snapshot::make(target, {
        package("foreign", "1", "1", binding('5'), {}),
    }));
  });

  return 0;
}
