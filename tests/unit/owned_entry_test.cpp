// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../fixtures/state.h"
#include "../support/test.h"

#include <cstdint>
#include <optional>

#include <libpkgstate/error.h>
#include <libpkgstate/owned_entry.h>

int main()
{
  using namespace pkgstate;

  const installed_object_timestamp timestamp(-7, 999999999U);
  TEST_EQ(timestamp.seconds(), std::int64_t{-7});
  TEST_EQ(timestamp.nanoseconds(), std::uint32_t{999999999U});
  TEST_THROWS(state_error, installed_object_timestamp(0, 1000000000U));

  const installed_device_number device(8, 17);
  TEST_EQ(device.major(), std::uint64_t{8});
  TEST_EQ(device.minor(), std::uint64_t{17});

  const package_path anchor = package_path::parse("usr/lib/libx.so");
  const installed_object_metadata regular(
      owned_object_kind::regular, 04755, 1000, 100,
      installed_object_timestamp(-2, 3), std::uint64_t{19},
      state_fixture::identity<installed_regular_content_identity>(10),
      std::nullopt, std::nullopt, anchor);
  TEST_EQ(regular.kind(), owned_object_kind::regular);
  TEST_EQ(regular.mode(), std::uint32_t{04755});
  TEST_EQ(regular.uid(), std::uint64_t{1000});
  TEST_EQ(regular.gid(), std::uint64_t{100});
  TEST_EQ(*regular.size(), std::uint64_t{19});
  TEST(regular.regular_content().has_value());
  TEST_EQ(*regular.hardlink_anchor(), anchor);
  TEST(!regular.symlink_target());
  TEST(!regular.device());

  const installed_object_metadata directory(
      owned_object_kind::directory, 01755, 0, 0,
      installed_object_timestamp(1, 2));
  TEST_EQ(directory.kind(), owned_object_kind::directory);
  TEST(!directory.size());

  const installed_object_metadata symlink(
      owned_object_kind::symlink, 0777, 1, 2,
      installed_object_timestamp(4, 5), std::nullopt, std::nullopt,
      std::string("../target"));
  TEST_EQ(*symlink.symlink_target(), std::string("../target"));

  const installed_object_metadata character(
      owned_object_kind::character_device, 0600, 0, 6,
      installed_object_timestamp(7, 8), std::nullopt, std::nullopt,
      std::nullopt, installed_device_number(1, 3));
  TEST_EQ(character.device()->major(), std::uint64_t{1});

  const installed_object_metadata block(
      owned_object_kind::block_device, 0600, 0, 6,
      installed_object_timestamp(7, 8), std::nullopt, std::nullopt,
      std::nullopt, installed_device_number(8, 1));
  TEST_EQ(block.device()->minor(), std::uint64_t{1});

  for (const owned_object_kind kind : {
           owned_object_kind::fifo,
           owned_object_kind::socket,
           owned_object_kind::other,
       })
  {
    const installed_object_metadata value(
        kind, 0640, 9, 10, installed_object_timestamp(11, 12));
    TEST_EQ(value.kind(), kind);
  }

  TEST_THROWS(
      state_error,
      installed_object_metadata(
          static_cast<owned_object_kind>(0), 0644, 0, 0,
          installed_object_timestamp(0, 0)));
  TEST_THROWS(
      state_error,
      installed_object_metadata(
          owned_object_kind::regular, 0100000U | 0644U, 0, 0,
          installed_object_timestamp(0, 0), std::uint64_t{1},
          state_fixture::identity<installed_regular_content_identity>(20)));
  TEST_THROWS(
      state_error,
      installed_object_metadata(
          owned_object_kind::regular, 0644, 0, 0,
          installed_object_timestamp(0, 0), std::uint64_t{1}));
  TEST_THROWS(
      state_error,
      installed_object_metadata(
          owned_object_kind::regular, 0644, 0, 0,
          installed_object_timestamp(0, 0), std::nullopt,
          state_fixture::identity<installed_regular_content_identity>(21)));
  TEST_THROWS(
      state_error,
      installed_object_metadata(
          owned_object_kind::directory, 0755, 0, 0,
          installed_object_timestamp(0, 0), std::uint64_t{1}));
  TEST_THROWS(
      state_error,
      installed_object_metadata(
          owned_object_kind::character_device, 0600, 0, 0,
          installed_object_timestamp(0, 0)));
  TEST_THROWS(
      state_error,
      installed_object_metadata(
          owned_object_kind::directory, 0755, 0, 0,
          installed_object_timestamp(0, 0), std::nullopt, std::nullopt,
          std::nullopt, std::nullopt, package_path::parse("anchor")));
  TEST_THROWS(
      state_error,
      installed_object_metadata(
          owned_object_kind::symlink, 0777, 0, 0,
          installed_object_timestamp(0, 0), std::nullopt, std::nullopt,
          std::string("bad\nlink")));

  const rejected_object_reference rejected(
      rejected_object_side::prior_installed,
      state_fixture::identity<rejected_object_identity>(30));
  TEST_EQ(rejected.side(), rejected_object_side::prior_installed);
  TEST_THROWS(
      state_error,
      rejected_object_reference(
          static_cast<rejected_object_side>(0),
          state_fixture::identity<rejected_object_identity>(31)));

  const owned_entry entry = owned_entry::make(
      package_path::parse("usr/bin/example"), regular,
      active_object_origin::retained_existing, rejected);
  TEST_EQ(entry.path().string(), std::string("usr/bin/example"));
  TEST_EQ(entry.kind(), owned_object_kind::regular);
  TEST_EQ(entry.origin(), active_object_origin::retained_existing);
  TEST(entry.rejected().has_value());
  TEST_THROWS(
      state_error,
      owned_entry::make(
          package_path::parse("usr/bin/example"), regular,
          static_cast<active_object_origin>(0)));
}
