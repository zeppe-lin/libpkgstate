// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include "native_fixture.h"
#include "test.h"

#include <libpkgstate/error.h>

int main()
{
  using namespace pkgstate;
  const installation_receipt receipt = native_fixture::receipt();
  TEST_EQ(receipt.schema_version(), installation_receipt_schema_version);
  TEST_EQ(receipt.manifest().size(), std::size_t{2});
  TEST_EQ(receipt.manifest().front().path().string(),
          std::string("etc/example.conf"));
  TEST(receipt.manifest().front().rejected().has_value());
  TEST_EQ(receipt.control().source().snapshot(),
          native_fixture::source().snapshot());

  TEST_EQ(receipt.manifest().back().object().mtime().seconds(),
          std::int64_t{-1});
  TEST(receipt.manifest().back().object().regular_content().has_value());

  TEST_THROWS(state_error, installed_object_timestamp(0, 1000000000U));
  TEST_THROWS(
      state_error,
      installed_object_metadata(
          owned_object_kind::regular, 0100000U | 0644U, 0, 0,
          installed_object_timestamp(0, 0), std::uint64_t{1},
          native_fixture::identity<installed_regular_content_identity>(81)));
  TEST_THROWS(
      state_error,
      installed_object_metadata(
          owned_object_kind::regular, 0644, 0, 0,
          installed_object_timestamp(0, 0), std::uint64_t{1}));
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
          owned_object_kind::symlink, 0777, 0, 0,
          installed_object_timestamp(0, 0), std::nullopt, std::nullopt,
          std::string("bad\nlink")));
  TEST_THROWS(
      state_error,
      installed_object_metadata(
          owned_object_kind::directory, 0755, 0, 0,
          installed_object_timestamp(0, 0), std::nullopt, std::nullopt,
          std::nullopt, std::nullopt, package_path::parse("anchor")));

  const installed_object_metadata symlink(
      owned_object_kind::symlink, 0777, 1, 2,
      installed_object_timestamp(-2, 3), std::nullopt, std::nullopt,
      std::string("../target"));
  TEST_EQ(*symlink.symlink_target(), std::string("../target"));

  const installed_object_metadata device(
      owned_object_kind::block_device, 0600, 1, 2,
      installed_object_timestamp(4, 5), std::nullopt, std::nullopt,
      std::nullopt, installed_device_number(8, 1));
  TEST(device.device().has_value());
  TEST_EQ(device.device()->major(), std::uint64_t{8});

  const owned_entry duplicate = owned_entry::make(
      package_path::parse("same"), native_fixture::regular_object(80),
      active_object_origin::incoming_payload);
  TEST_THROWS(
      state_error,
      installation_receipt::make(
          native_fixture::control(), native_fixture::target(),
          {duplicate, duplicate},
          native_fixture::identity<operation_plan_identity>(90),
          native_fixture::identity<application_evidence_identity>(91)));
}
