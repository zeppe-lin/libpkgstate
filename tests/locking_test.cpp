// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/legacy_text_store.h>

#include <cerrno>
#include <filesystem>

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include "temp_directory.h"
#include "test.h"

int
main()
{
  temp_directory temporary;
  const std::filesystem::path database = temporary.path() / "db";
  write_text(database, "base\n1\nusr/bin/base\n\n");

  pkgstate::legacy_text_store store(database);

  const int fd =
      ::open(temporary.path().c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  CHECK(fd != -1);
  CHECK(::flock(fd, LOCK_EX | LOCK_NB) == 0);

  check_throws<pkgstate::store_error>([&] {
    static_cast<void>(store.read());
  });
  check_throws<pkgstate::store_error>([&] {
    static_cast<void>(store.begin_write());
  });

  CHECK(::flock(fd, LOCK_UN) == 0);
  CHECK(::close(fd) == 0);

  CHECK(store.read().size() == 1);
  auto transaction = store.begin_write();
  CHECK(transaction->current().size() == 1);

  return 0;
}
