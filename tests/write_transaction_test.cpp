/*
 * Copyright (C) 2026 Alexandr Savca
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <libpkgstate/legacy_text_store.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <sys/stat.h>

#include "temp_directory.h"
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
  temp_directory temporary;
  const std::filesystem::path database = temporary.path() / "db";
  const std::string initial =
      "base\n"
      "1.0-1\n"
      "etc/base.conf\n"
      "usr/bin/base\n"
      "\n"
      "old\n"
      "1\n"
      "usr/bin/old\n"
      "\n";
  write_text(database, initial);

  pkgstate::legacy_text_store backend(database);
  pkgstate::store& abstract_store = backend;

  std::unique_ptr<pkgstate::write_transaction> transaction =
      abstract_store.begin_write();
  CHECK(!transaction->committed());
  CHECK(transaction->current().size() == 2);

  transaction->put(package(
      "base", "2.0-1", {directory("usr/share/base"), file("usr/bin/base")}));
  transaction->put(package("new", "1", {}));
  CHECK(transaction->erase("old"));
  CHECK(!transaction->erase("missing"));

  const pkgstate::snapshot pending = transaction->current();
  CHECK(pending.size() == 2);
  CHECK(pending.find_package("base")->identity().version() == "2.0-1");
  CHECK(pending.find_package("new")->manifest().empty());

  check_throws<pkgstate::store_error>([&] {
    static_cast<void>(abstract_store.read());
  });
  check_throws<pkgstate::store_error>([&] {
    static_cast<void>(abstract_store.begin_write());
  });

  transaction->commit();
  CHECK(transaction->committed());

  const pkgstate::snapshot durable = abstract_store.read();
  CHECK(durable.size() == 2);
  CHECK(durable.find_package("old") == nullptr);
  CHECK(durable.find_package("base")->manifest()[0].path.string() == "usr/bin/base");
  CHECK(durable.find_package("base")->manifest()[1].path.string() == "usr/share/base");

  const std::string expected =
      "base\n"
      "2.0-1\n"
      "usr/bin/base\n"
      "usr/share/base/\n"
      "\n"
      "new\n"
      "1\n"
      "\n";
  CHECK(read_text(database) == expected);
  CHECK(read_text(database.string() + ".backup") == initial);

  struct stat metadata {};
  CHECK(::stat(database.c_str(), &metadata) == 0);
  CHECK((metadata.st_mode & 0777) == 0444);

  check_throws<pkgstate::transaction_error>([&] {
    transaction->commit();
  });
  check_throws<pkgstate::transaction_error>([&] {
    transaction->put(package("later", "1", {}));
  });
  check_throws<pkgstate::transaction_error>([&] {
    static_cast<void>(transaction->erase("base"));
  });

  check_throws<pkgstate::identity_error>([&] {
    auto invalid = abstract_store.begin_write();
    static_cast<void>(invalid->erase("bad\nname"));
  });

  const std::string before_rollback = read_text(database);
  {
    auto rollback = abstract_store.begin_write();
    CHECK(rollback->erase("base"));
    rollback->put(package("uncommitted", "1", {file("tmp/ghost")}));
  }
  CHECK(read_text(database) == before_rollback);
  CHECK(abstract_store.read().find_package("base") != nullptr);
  CHECK(abstract_store.read().find_package("uncommitted") == nullptr);

  return 0;
}
