// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../fixtures/state.h"
#include "../support/test.h"

int main()
{
  using namespace pkgstate;

  const state_target_binding binding = state_fixture::target();
  const installed_package value = state_fixture::package("example", 20, binding);
  TEST_EQ(value.receipt(), state_fixture::receipt("example", 20, binding));
  TEST_EQ(value.release(), value.control().source().release());
  TEST_EQ(value.target_binding(), binding);
  TEST_EQ(value.size(), value.manifest().size());

  const package_path binary = package_path::parse("usr/bin/example");
  TEST(value.owns(binary));
  TEST(value.find(binary) != nullptr);
  TEST_EQ(value.find(binary)->path(), binary);
  TEST(!value.owns(package_path::parse("usr/bin/missing")));
  TEST(value.find(package_path::parse("usr/bin/missing")) == nullptr);

  const installed_package empty = state_fixture::empty_package("empty", 40, binding);
  TEST_EQ(empty.size(), std::size_t{0});
  TEST(empty.manifest().empty());
  TEST(!empty.owns(binary));

  const installed_package changed = state_fixture::package("example", 21, binding);
  TEST(value.identity() != changed.identity());
}
