// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/installed_package.h>
#include <libpkgstate/legacy_installed_package.h>
#include <libpkgstate/legacy_snapshot.h>
#include <libpkgstate/snapshot.h>

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

pkgstate::legacy_installed_package
package(const char* name,
        const char* version,
        std::vector<pkgstate::owned_entry> manifest)
{
  return pkgstate::legacy_installed_package(
      pkgstate::package_identity::make(name, version),
      std::move(manifest));
}

} // namespace

int
main()
{
  static_assert(!std::is_default_constructible_v<
                pkgstate::legacy_installed_package>);
  static_assert(!std::is_convertible_v<
                pkgstate::legacy_installed_package,
                pkgstate::installed_package>);
  static_assert(!std::is_convertible_v<
                pkgstate::legacy_snapshot,
                pkgstate::snapshot>);

  const pkgstate::legacy_installed_package base = package(
      "base", "1.0-release-with-hyphens",
      {file("usr/bin/base"), directory("etc/base")});
  CHECK(base.identity().name() == "base");
  CHECK(base.identity().version() == "1.0-release-with-hyphens");
  CHECK(base.manifest()[0].path.string() == "etc/base");
  CHECK(base.manifest()[1].path.string() == "usr/bin/base");

  check_throws<pkgstate::state_error>([] {
    static_cast<void>(package(
        "duplicate", "1",
        {file("usr/bin/tool"), directory("usr/bin/tool")}));
  });
  check_throws<pkgstate::state_error>([] {
    static_cast<void>(pkgstate::legacy_installed_package(
        pkgstate::package_identity::make("invalid", "1"),
        {{path("usr/bin/invalid"),
          static_cast<pkgstate::owned_entry_type>(99)}}));
  });

  const pkgstate::legacy_snapshot state({
      package("zlib", "1.3-1", {file("usr/lib/libz.so")}),
      package("base", "1.0-1", {directory("usr/share/common")}),
      package("other", "2-1", {directory("usr/share/common")}),
  });

  CHECK(state.packages()[0].identity().name() == "base");
  CHECK(state.packages()[1].identity().name() == "other");
  CHECK(state.packages()[2].identity().name() == "zlib");
  CHECK(state.find_package("other") != nullptr);
  CHECK(state.find_package("missing") == nullptr);

  const auto owners = state.owners(path("usr/share/common"));
  CHECK(owners.size() == 2);
  CHECK(owners[0]->identity().name() == "base");
  CHECK(owners[1]->identity().name() == "other");

  check_throws<pkgstate::state_error>([] {
    static_cast<void>(pkgstate::legacy_snapshot({
        package("same", "1", {}),
        package("same", "2", {}),
    }));
  });

  return 0;
}
