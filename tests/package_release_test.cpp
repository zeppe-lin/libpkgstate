// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/package_release.h>

#include <string>
#include <type_traits>

#include "test.h"

int
main()
{
  static_assert(!std::is_default_constructible_v<pkgstate::package_release>);
  static_assert(!std::is_constructible_v<
                pkgstate::package_release,
                pkgstate::package_release_identity,
                std::string,
                std::string,
                std::string>);

  const pkgstate::package_release base =
      pkgstate::package_release::make("base", "1.0", "1");

  CHECK(base.name() == "base");
  CHECK(base.version() == "1.0");
  CHECK(base.release() == "1");
  CHECK(base.identity().string() ==
        "v1:sha256:"
        "e209d12ba6792150e5de40fe2e8416bb77284d5438e2cea1a33cec3b8575fcca");

  const pkgstate::package_release same =
      pkgstate::package_release::make("base", "1.0", "1");
  CHECK(base == same);
  CHECK(!(base != same));

  const pkgstate::package_release newer_release =
      pkgstate::package_release::make("base", "1.0", "2");
  CHECK(base != newer_release);
  CHECK(base.identity() != newer_release.identity());
  CHECK(base < newer_release);

  const pkgstate::package_release split_left =
      pkgstate::package_release::make("ab", "c", "d");
  const pkgstate::package_release split_right =
      pkgstate::package_release::make("a", "bc", "d");
  CHECK(split_left.identity() != split_right.identity());

  check_throws<pkgstate::identity_error>([] {
    static_cast<void>(pkgstate::package_release::make("", "1", "1"));
  });
  check_throws<pkgstate::identity_error>([] {
    static_cast<void>(pkgstate::package_release::make("base", "", "1"));
  });
  check_throws<pkgstate::identity_error>([] {
    static_cast<void>(pkgstate::package_release::make("base", "1", ""));
  });
  check_throws<pkgstate::identity_error>([] {
    static_cast<void>(pkgstate::package_release::make("bad\nname", "1", "1"));
  });
  check_throws<pkgstate::identity_error>([] {
    static_cast<void>(pkgstate::package_release::make("base", "1\r2", "1"));
  });
  check_throws<pkgstate::identity_error>([] {
    const std::string release("1\0dirty", 7);
    static_cast<void>(pkgstate::package_release::make("base", "1", release));
  });

  return 0;
}
