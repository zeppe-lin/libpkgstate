// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/libpkgstate.h>

#include <string>
#include <vector>

#include "test.h"

namespace {

pkgimage::package_path
path(const char* value)
{
  return pkgimage::package_path::parse(value);
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

pkgstate::installed_package
package(const char* name,
        const char* version,
        std::vector<pkgstate::owned_entry> manifest)
{
  return pkgstate::installed_package(
      pkgstate::package_identity::make(name, version), std::move(manifest));
}

} // namespace

int
main()
{
  const pkgstate::package_identity identity =
      pkgstate::package_identity::make("base", "1.0-1");
  CHECK(identity.name() == "base");
  CHECK(identity.version() == "1.0-1");

  check_throws<pkgstate::identity_error>([] {
    static_cast<void>(pkgstate::package_identity::make("", "1"));
  });
  check_throws<pkgstate::identity_error>([] {
    static_cast<void>(pkgstate::package_identity::make("base", ""));
  });
  check_throws<pkgstate::identity_error>([] {
    static_cast<void>(pkgstate::package_identity::make("bad\nname", "1"));
  });
  check_throws<pkgstate::identity_error>([] {
    const std::string version("1\0dirty", 7);
    static_cast<void>(pkgstate::package_identity::make("base", version));
  });

  const pkgstate::installed_package base = package(
      "base", "1.0-1", {file("usr/bin/base"), directory("etc/base.conf")});
  CHECK(base.size() == 2);
  CHECK(base.manifest()[0].path.string() == "etc/base.conf");
  CHECK(base.manifest()[1].path.string() == "usr/bin/base");
  CHECK(base.owns(path("usr/bin/base")));
  CHECK(base.find(path("etc/base.conf"))->type ==
        pkgstate::owned_entry_type::directory);
  CHECK(!base.owns(path("usr/bin/other")));

  const pkgstate::installed_package empty = package("empty", "1", {});
  CHECK(empty.size() == 0);

  check_throws<pkgstate::state_error>([] {
    static_cast<void>(package(
        "duplicate", "1", {file("usr/bin/tool"), directory("usr/bin/tool")}));
  });

  pkgstate::snapshot state({
      package("zlib", "1.3", {file("usr/lib/libz.so")}),
      package("base", "1.0-1",
              {file("usr/bin/base"), directory("usr/share/common")}),
      package("other", "2", {directory("usr/share/common")}),
  });

  CHECK(state.size() == 3);
  CHECK(state.packages()[0].identity().name() == "base");
  CHECK(state.packages()[1].identity().name() == "other");
  CHECK(state.packages()[2].identity().name() == "zlib");
  CHECK(state.find_package("other") != nullptr);
  CHECK(state.find_package("missing") == nullptr);
  CHECK(state.is_owned(path("usr/share/common")));
  CHECK(!state.is_owned(path("usr/share/absent")));

  const auto owners = state.owners(path("usr/share/common"));
  CHECK(owners.size() == 2);
  CHECK(owners[0]->identity().name() == "base");
  CHECK(owners[1]->identity().name() == "other");

  check_throws<pkgstate::state_error>([] {
    static_cast<void>(pkgstate::snapshot({
        package("same", "1", {}),
        package("same", "2", {}),
    }));
  });

  return 0;
}
