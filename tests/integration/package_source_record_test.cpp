// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/state.h"
#include "../support/test.h"

#include <libpkgstate/error.h>

int main()
{
  using namespace pkgstate;
  const package_source_record first = state_fixture::source();
  const package_source_record same = state_fixture::source();
  const package_source_record changed = state_fixture::source("example", 21);

  TEST_EQ(first, same);
  TEST_NE(first.identity(), changed.identity());
  TEST_EQ(first.runtime_requirements().front().package().name(),
          std::string("libfoo"));
  TEST(first.lifecycle(lifecycle_action::pre_remove) != nullptr);
  TEST_EQ(first.lifecycle_requirements(lifecycle_action::post_install).size(),
          std::size_t{1});
  TEST_EQ(first.selected_profiles().front().profile().name(),
          std::string("@toolchain"));
  TEST_EQ(first.architectures().selected_target().name(),
          std::string("x86_64"));
}
