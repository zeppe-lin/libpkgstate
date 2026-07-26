// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "native_fixture.h"
#include "test.h"

#include <libpkgstate/error.h>

int main()
{
  using namespace pkgstate;
  const state_target_binding binding = native_fixture::target();
  installed_package alpha = native_fixture::package("alpha", 30, binding);
  installed_package beta = native_fixture::package("beta", 50, binding);

  const snapshot forward = snapshot::make(binding, {alpha, beta});
  const snapshot reverse = snapshot::make(binding, {beta, alpha});
  TEST_EQ(forward.identity(), reverse.identity());
  TEST_EQ(forward.ownership_identity(), reverse.ownership_identity());
  TEST_EQ(forward.packages().front().release().name(), std::string("alpha"));
  TEST(forward.is_owned(package_path::parse("usr/bin/example")));
  TEST_EQ(forward.owners(package_path::parse("usr/bin/example")).size(),
          std::size_t{2});

  TEST_THROWS(state_error, snapshot::make(binding, {alpha, alpha}));
  TEST_THROWS(state_error,
              snapshot::make(native_fixture::target(100), {alpha}));
}
