// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../fixtures/state.h"
#include "../support/test.h"

#include <libpkgstate/generation_codec.h>

#include <string>
#include <vector>

namespace {

std::string bytes(const std::vector<std::uint8_t>& value)
{
  return std::string(reinterpret_cast<const char*>(value.data()), value.size());
}

} // namespace

int main()
{
  using namespace pkgstate;
  static_assert(canonical_generation_storage_version == 1);
  TEST_EQ(canonical_generation_storage_format,
          std::string_view("libpkgstate-generation-v1"));

  const state_target_binding binding = state_fixture::target();
  const auto binding_encoded = encode_generation_binding(binding);
  const std::string binding_body = bytes(binding_encoded);
  TEST_EQ(decode_generation_binding(binding_body), binding);
  TEST_EQ(bytes(encode_generation_binding(decode_generation_binding(binding_body))),
          binding_body);
  TEST_THROWS(store_error,
              decode_generation_binding(binding_body.substr(0, binding_body.size() - 1)));
  std::string binding_trailing = binding_body + "x";
  TEST_THROWS(store_error, decode_generation_binding(binding_trailing));
  std::string binding_bad_magic = binding_body;
  binding_bad_magic[0] = static_cast<char>(binding_bad_magic[0] ^ 0x01);
  TEST_THROWS(store_error, decode_generation_binding(binding_bad_magic));

  const snapshot empty = snapshot::make(binding);
  const std::string empty_body = bytes(encode_generation_snapshot(empty));
  const snapshot empty_decoded = decode_generation_snapshot(empty_body);
  TEST_EQ(empty_decoded.identity(), empty.identity());
  TEST(empty_decoded.packages().empty());

  const snapshot ordinary = snapshot::make(
      binding, {state_fixture::package("example", 20, binding)});
  const std::string ordinary_body = bytes(encode_generation_snapshot(ordinary));
  const snapshot ordinary_decoded = decode_generation_snapshot(ordinary_body);
  TEST_EQ(ordinary_decoded.identity(), ordinary.identity());
  TEST_EQ(ordinary_decoded.packages(), ordinary.packages());
  TEST_EQ(bytes(encode_generation_snapshot(ordinary_decoded)), ordinary_body);

  const snapshot rich = snapshot::make(
      binding, {state_fixture::rich_package("rich", 60, binding),
                state_fixture::empty_package("empty", 100, binding)});
  const std::string rich_body = bytes(encode_generation_snapshot(rich));
  const snapshot rich_decoded = decode_generation_snapshot(rich_body);
  TEST_EQ(rich_decoded.identity(), rich.identity());
  TEST_EQ(rich_decoded.target_binding(), rich.target_binding());
  TEST_EQ(rich_decoded.packages(), rich.packages());
  TEST_EQ(bytes(encode_generation_snapshot(rich_decoded)), rich_body);

  std::string truncated = rich_body.substr(0, rich_body.size() - 1);
  TEST_THROWS(store_error, decode_generation_snapshot(truncated));
  std::string trailing = rich_body + "x";
  TEST_THROWS(store_error, decode_generation_snapshot(trailing));
  std::string bad_magic = rich_body;
  bad_magic[0] = static_cast<char>(bad_magic[0] ^ 0x01);
  TEST_THROWS(store_error, decode_generation_snapshot(bad_magic));

  std::string legacy_version = ordinary_body;
  constexpr std::size_t snapshot_version_offset = 18;
  TEST(legacy_version.size() > snapshot_version_offset + 1);
  legacy_version[snapshot_version_offset] = '\0';
  legacy_version[snapshot_version_offset + 1] = '\x02';
  TEST_THROWS(store_error, decode_generation_snapshot(legacy_version));
}
