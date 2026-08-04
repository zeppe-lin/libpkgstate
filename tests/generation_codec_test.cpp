// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "native_fixture.h"
#include "test.h"

#include <libpkgstate/generation_codec.h>

#include <string>

int main()
{
  using namespace pkgstate;
  static_assert(canonical_generation_storage_version == 4);
  TEST_EQ(canonical_generation_storage_format,
          std::string_view("libpkgstate-generation-v4"));

  const state_target_binding binding = native_fixture::target();
  const std::vector<std::uint8_t> binding_bytes =
      encode_generation_binding(binding);
  TEST_EQ(decode_generation_binding(
              std::string_view(
                  reinterpret_cast<const char*>(binding_bytes.data()),
                  binding_bytes.size())),
          binding);

  const snapshot state = snapshot::make(
      binding, {native_fixture::package("example", 20, binding)});
  const std::vector<std::uint8_t> snapshot_bytes =
      encode_generation_snapshot(state);
  const snapshot decoded = decode_generation_snapshot(
      std::string_view(
          reinterpret_cast<const char*>(snapshot_bytes.data()),
          snapshot_bytes.size()));
  TEST_EQ(decoded.identity(), state.identity());
  TEST_EQ(decoded.target_binding(), state.target_binding());
  TEST_EQ(decoded.packages(), state.packages());

  std::string legacy_version(
      reinterpret_cast<const char*>(snapshot_bytes.data()),
      snapshot_bytes.size());
  constexpr std::size_t snapshot_version_offset = 18;
  legacy_version[snapshot_version_offset] = '\0';
  legacy_version[snapshot_version_offset + 1] = '\x03';
  TEST_THROWS(store_error, decode_generation_snapshot(legacy_version));

  std::string damaged(
      reinterpret_cast<const char*>(snapshot_bytes.data()),
      snapshot_bytes.size());
  damaged.push_back('x');
  TEST_THROWS(store_error, decode_generation_snapshot(damaged));
}
