// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/error.h>

#include "../support/test.h"

#include <stdexcept>
#include <string>
#include <type_traits>

int main()
{
  static_assert(std::has_virtual_destructor_v<pkgstate::error>);
  static_assert(std::has_virtual_destructor_v<pkgstate::identity_error>);
  static_assert(std::has_virtual_destructor_v<pkgstate::path_error>);
  static_assert(std::has_virtual_destructor_v<pkgstate::state_error>);
  static_assert(std::has_virtual_destructor_v<pkgstate::store_error>);

  const auto catches_as_base = [](const auto& failure) {
    try
    {
      throw failure;
    }
    catch (const pkgstate::error& caught)
    {
      CHECK(std::string(caught.what()) == "failure");
      return;
    }
    CHECK(false);
  };

  catches_as_base(pkgstate::identity_error("failure"));
  catches_as_base(pkgstate::path_error("failure"));
  catches_as_base(pkgstate::state_error("failure"));
  catches_as_base(pkgstate::store_error("failure"));
  return 0;
}
