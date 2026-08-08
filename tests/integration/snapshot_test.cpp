// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/state.h"
#include "../support/test.h"

#include <libpkgstate/error.h>

namespace {

pkgstate::installed_package one_path_package(
    const std::string& name, std::uint8_t seed,
    const pkgstate::state_target_binding& binding,
    pkgstate::installed_object_metadata object)
{
  auto control = state_fixture::control(name, seed);
  std::vector<pkgstate::owned_entry> manifest;
  manifest.push_back(state_fixture::entry("usr/share/shared", std::move(object)));
  return state_fixture::package_from_receipt(state_fixture::receipt_from_control(
      std::move(control), seed, binding, std::move(manifest)));
}

} // namespace

int main()
{
  using namespace pkgstate;
  const state_target_binding binding = state_fixture::target();
  const installed_package alpha = state_fixture::package("alpha", 30, binding);
  const installed_package beta = state_fixture::package("beta", 50, binding);
  const installed_package empty = state_fixture::empty_package("empty", 70, binding);

  const snapshot forward = snapshot::make(binding, {alpha, beta, empty});
  const snapshot reverse = snapshot::make(binding, {empty, beta, alpha});
  TEST_EQ(forward.identity(), reverse.identity());
  TEST_EQ(forward.ownership_identity(), reverse.ownership_identity());
  TEST_EQ(forward.size(), std::size_t{3});
  TEST_EQ(forward.packages().front().release().name(), std::string("alpha"));
  TEST(forward.find_package("empty") != nullptr);
  TEST_EQ(forward.find_package("empty")->size(), std::size_t{0});
  TEST(forward.find_package("missing") == nullptr);

  const package_path shared = package_path::parse("usr/bin/example");
  TEST(forward.is_owned(shared));
  const auto owners = forward.owners(shared);
  TEST_EQ(owners.size(), std::size_t{2});
  TEST_EQ(owners[0]->release().name(), std::string("alpha"));
  TEST_EQ(owners[1]->release().name(), std::string("beta"));
  TEST(!forward.is_owned(package_path::parse("usr/bin/missing")));
  TEST(forward.owners(package_path::parse("usr/bin/missing")).empty());

  TEST_THROWS(state_error, snapshot::make(binding, {alpha, alpha}));
  TEST_THROWS(state_error,
              snapshot::make(state_fixture::target(100), {alpha}));

  const installed_package shared_a = one_path_package(
      "shared-a", 90, binding, state_fixture::regular_object(91));
  const installed_package shared_b_same = one_path_package(
      "shared-b", 100, binding, state_fixture::regular_object(91));
  const snapshot compatible = snapshot::make(binding, {shared_b_same, shared_a});
  TEST_EQ(compatible.owners(package_path::parse("usr/share/shared")).size(),
          std::size_t{2});

  const installed_package shared_b_different = one_path_package(
      "shared-c", 110, binding, state_fixture::regular_object(111));
  const snapshot contradictory =
      snapshot::make(binding, {shared_a, shared_b_different});
  TEST_EQ(contradictory.owners(package_path::parse("usr/share/shared")).size(),
          std::size_t{2});
  TEST(contradictory.identity() != compatible.identity());
  TEST(contradictory.ownership_identity() != compatible.ownership_identity());
}
