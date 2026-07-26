// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "native_fixture.h"
#include "test.h"

#include <cstdint>

#include <libpkgstate/package_release.h>

int main()
{
  const auto identity =
      native_fixture::identity<pkgstate::package_release_identity>(1);
  const pkgstate::package_release release(
      identity, pkgstate::package_reference("base"), "1.0", 2);
  TEST_EQ(release.identity(), identity);
  TEST_EQ(release.name(), "base");
  TEST_EQ(release.version(), "1.0");
  TEST_EQ(release.release(), std::uint32_t{2});
  TEST_EQ(release.version_release(), "1.0-2");

  TEST_THROWS(pkgstate::state_error,
              pkgstate::package_release(
                  identity, pkgstate::package_reference("base"), "", 1));
  TEST_THROWS(pkgstate::state_error,
              pkgstate::package_release(
                  identity, pkgstate::package_reference("base"), "1/2", 1));
  TEST_THROWS(pkgstate::state_error,
              pkgstate::package_release(
                  identity, pkgstate::package_reference("base"), "1.0", 0));
}
