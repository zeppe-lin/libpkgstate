// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../support/test.h"

#include <array>
#include <string>

#include <libpkgstate/digest.h>

namespace {

template<typename Identity>
Identity identity(std::uint8_t seed)
{
  pkgstate::sha256_digest_bytes bytes{};
  bytes.fill(seed);
  return Identity::from_sha256(bytes);
}

void round_trips_owned_and_referenced_identities()
{
  const auto owned = identity<pkgstate::installed_package_identity>(0x5a);
  TEST_EQ(owned.representation_version(), std::uint16_t{1});
  TEST_EQ(owned.algorithm(), pkgstate::digest_algorithm::sha256);
  TEST_EQ(owned.bytes().size(), pkgstate::sha256_digest_size);
  TEST_EQ(pkgstate::installed_package_identity::parse(owned.string()), owned);
  TEST_EQ(pkgstate::installed_package_identity::canonical_domain(),
          "pkgstate/installed-package/1");

  const auto referenced = identity<pkgstate::package_release_identity>(0x2b);
  TEST_EQ(pkgstate::package_release_identity::parse(referenced.string()),
          referenced);
  TEST_EQ(referenced.bytes().size(), pkgstate::sha256_digest_size);
}

void rejects_invalid_wire_forms()
{
  TEST_THROWS(pkgstate::digest_error,
              pkgstate::installed_package_identity::parse(""));
  TEST_THROWS(pkgstate::digest_error,
              pkgstate::installed_package_identity::parse("v2:sha256:" +
                  std::string(64, '0')));
  TEST_THROWS(pkgstate::digest_error,
              pkgstate::installed_package_identity::parse("v1:md5:" +
                  std::string(64, '0')));
  TEST_THROWS(pkgstate::digest_error,
              pkgstate::installed_package_identity::parse("v1:sha256:00"));
  TEST_THROWS(pkgstate::digest_error,
              pkgstate::installed_package_identity::parse("v1:sha256:" +
                  std::string(64, 'z')));
}

} // namespace

int main()
{
  round_trips_owned_and_referenced_identities();
  rejects_invalid_wire_forms();
}
