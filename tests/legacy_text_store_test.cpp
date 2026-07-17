/*
 * Copyright (C) 2026 Alexandr Savca
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <libpkgstate/legacy_text_store.h>

#include <filesystem>
#include <string>

#include "temp_directory.h"
#include "test.h"

namespace {

pkgimage::package_path
path(const char* value)
{
  return pkgimage::package_path::parse(value);
}

void
expect_store_error(const std::filesystem::path& database,
                   const std::string& contents)
{
  write_text(database, contents);
  pkgstate::legacy_text_store store(database);
  check_throws<pkgstate::store_error>([&] {
    static_cast<void>(store.read());
  });
}

} // namespace

int
main()
{
  temp_directory temporary;
  const std::filesystem::path database = temporary.path() / "db";

  write_text(database,
             "\n"
             "zlib\n"
             "1.3-1\n"
             "usr/lib/libz.so\n"
             "\n"
             "\n"
             "base\n"
             "1.0-1\n"
             "./usr/bin/base\n"
             "usr/share/doc/\n"
             "\n"
             "empty\n"
             "1\n"
             "\n");

  pkgstate::legacy_text_store store(database);
  CHECK(store.database_path() == database);

  const pkgstate::snapshot state = store.read();
  CHECK(state.size() == 3);
  CHECK(state.packages()[0].identity().name() == "base");
  CHECK(state.packages()[1].identity().name() == "empty");
  CHECK(state.packages()[2].identity().name() == "zlib");
  CHECK(state.find_package("base")->manifest()[0].path.string() == "usr/bin/base");
  CHECK(state.find_package("base")->manifest()[1].path.string() == "usr/share/doc");
  CHECK(state.find_package("base")->manifest()[1].type ==
        pkgstate::owned_entry_type::directory);
  CHECK(state.find_package("empty")->manifest().empty());
  CHECK(state.owners(path("usr/lib/libz.so")).size() == 1);

  write_text(database,
             "base\n"
             "1.0-1\n"
             "usr/bin/base\n");
  CHECK(store.read().find_package("base") != nullptr);

  write_text(database, "");
  CHECK(store.read().size() == 0);

  expect_store_error(database, "base\n");
  expect_store_error(database, "base\n\n\n");
  expect_store_error(database,
                     "base\n1\n../escape\n\n");
  expect_store_error(database,
                     "base\n1\nusr/bin/base\n./usr/bin/base\n\n");
  expect_store_error(database,
                     "base\n1\nusr/bin/base\n\nbase\n2\nusr/bin/new\n\n");

  std::filesystem::remove(database);
  check_throws<pkgstate::store_error>([&] {
    static_cast<void>(store.read());
  });

  check_throws<pkgstate::store_error>([] {
    static_cast<void>(pkgstate::legacy_text_store({}));
  });

  return 0;
}
