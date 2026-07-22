// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/error.h>
#include <libpkgstate/package_path.h>

#include <sstream>
#include <string>

#include "test.h"

int
main()
{
  const pkgstate::package_path path =
      pkgstate::package_path::parse("./usr//lib/./libpkgstate.so/");
  CHECK(path.string() == "usr/lib/libpkgstate.so");
  CHECK(path.filename() == "libpkgstate.so");

  const auto parent = path.parent();
  CHECK(parent.has_value());
  CHECK(parent->string() == "usr/lib");
  CHECK(parent->filename() == "lib");
  CHECK(!pkgstate::package_path::parse("usr").parent().has_value());

  const pkgstate::package_path usr = pkgstate::package_path::parse("usr");
  const pkgstate::package_path usr_lib =
      pkgstate::package_path::parse("usr/lib");
  const pkgstate::package_path usr_lib64 =
      pkgstate::package_path::parse("usr/lib64");
  CHECK(usr.is_ancestor_of(path));
  CHECK(usr_lib.is_ancestor_of(path));
  CHECK(!path.is_ancestor_of(path));
  CHECK(!usr_lib.is_ancestor_of(usr_lib64));

  CHECK(pkgstate::package_path::parse("a") <
        pkgstate::package_path::parse("b"));
  CHECK(pkgstate::package_path::parse("usr//lib") == usr_lib);
  CHECK(pkgstate::package_path::parse("usr/lib") != usr_lib64);

  std::ostringstream rendered;
  rendered << path;
  CHECK(rendered.str() == path.string());

  check_throws<pkgstate::path_error>([] {
    static_cast<void>(pkgstate::package_path::parse(""));
  });
  check_throws<pkgstate::path_error>([] {
    static_cast<void>(pkgstate::package_path::parse("/usr/bin/tool"));
  });
  check_throws<pkgstate::path_error>([] {
    static_cast<void>(pkgstate::package_path::parse("."));
  });
  check_throws<pkgstate::path_error>([] {
    static_cast<void>(pkgstate::package_path::parse("./"));
  });
  check_throws<pkgstate::path_error>([] {
    static_cast<void>(pkgstate::package_path::parse(".."));
  });
  check_throws<pkgstate::path_error>([] {
    static_cast<void>(pkgstate::package_path::parse("usr/../bin/tool"));
  });
  check_throws<pkgstate::path_error>([] {
    const std::string input("usr/bin\0tool", 12);
    static_cast<void>(pkgstate::package_path::parse(input));
  });
  check_throws<pkgstate::path_error>([] {
    static_cast<void>(pkgstate::package_path::parse("usr/bin\ntool"));
  });
  check_throws<pkgstate::path_error>([] {
    static_cast<void>(pkgstate::package_path::parse("usr/bin\rtool"));
  });

  return 0;
}
