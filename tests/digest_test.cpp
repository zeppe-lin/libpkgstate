// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/digest.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

#include "test.h"

namespace {

std::string
zeros()
{
  return std::string(pkgstate::sha256_digest_size * 2, '0');
}

template<typename Callable>
void
check_digest_error(pkgstate::digest_error_code expected, Callable&& callable)
{
  try
  {
    callable();
  }
  catch (const pkgstate::digest_error& failure)
  {
    CHECK(failure.code() == expected);
    return;
  }
  catch (const std::exception& failure)
  {
    std::cerr << "unexpected exception type: " << failure.what() << '\n';
    std::exit(EXIT_FAILURE);
  }

  std::cerr << "expected digest_error was not thrown\n";
  std::exit(EXIT_FAILURE);
}

} // namespace

int
main()
{
  static_assert(!std::is_same_v<pkgstate::package_release_identity,
                                pkgstate::installed_package_identity>);
  static_assert(!std::is_constructible_v<pkgstate::installed_package_identity,
                                         pkgstate::package_release_identity>);

  pkgstate::sha256_digest_bytes sequential{};
  for (std::size_t index = 0; index < sequential.size(); ++index)
    sequential[index] = static_cast<std::uint8_t>(index);

  const auto release =
      pkgstate::package_release_identity::from_sha256(sequential);
  CHECK(release.representation_version() == 1);
  CHECK(release.algorithm() == pkgstate::digest_algorithm::sha256);
  CHECK(release.bytes().size() == pkgstate::sha256_digest_size);
  CHECK(release.bytes().front() == 0);
  CHECK(release.bytes().back() == 31);
  CHECK(release.string() ==
        "v1:sha256:"
        "000102030405060708090a0b0c0d0e0f"
        "101112131415161718191a1b1c1d1e1f");

  const auto parsed =
      pkgstate::package_release_identity::parse(release.string());
  CHECK(parsed == release);

  pkgstate::sha256_digest_bytes larger = sequential;
  larger.back() = 32;
  CHECK(release < pkgstate::package_release_identity::from_sha256(larger));

  CHECK(pkgstate::package_release_identity::canonical_domain() ==
        "pkgstate/package-release/1");
  CHECK(pkgstate::installed_control_identity::canonical_domain() ==
        "pkgstate/installed-control/1");
  CHECK(pkgstate::installed_package_identity::canonical_domain() ==
        "pkgstate/installed-package/1");
  CHECK(pkgstate::ownership_inventory_identity::canonical_domain() ==
        "pkgstate/ownership-inventory/1");
  CHECK(pkgstate::installed_state_snapshot_identity::canonical_domain() ==
        "pkgstate/installed-snapshot/1");
  CHECK(pkgstate::state_publication_request_identity::canonical_domain() ==
        "pkgstate/publication-request/1");
  CHECK(pkgstate::state_publication_receipt_identity::canonical_domain() ==
        "pkgstate/publication-receipt/1");
  CHECK(pkgstate::legacy_import_request_identity::canonical_domain() ==
        "pkgstate/legacy-import-request/1");
  CHECK(pkgstate::legacy_import_receipt_identity::canonical_domain() ==
        "pkgstate/legacy-import-receipt/1");
  CHECK(pkgstate::legacy_package_observation_identity::canonical_domain() ==
        "pkgstate/legacy-package-observation/1");
  CHECK(pkgstate::legacy_snapshot_observation_identity::canonical_domain() ==
        "pkgstate/legacy-snapshot-observation/1");

  const std::array<std::string_view, 17> domains = {
      pkgstate::package_release_identity::canonical_domain(),
      pkgstate::installed_control_identity::canonical_domain(),
      pkgstate::installed_package_identity::canonical_domain(),
      pkgstate::ownership_inventory_identity::canonical_domain(),
      pkgstate::managed_target_identity::canonical_domain(),
      pkgstate::state_store_identity::canonical_domain(),
      pkgstate::root_view_identity::canonical_domain(),
      pkgstate::state_backend_identity::canonical_domain(),
      pkgstate::publication_domain_identity::canonical_domain(),
      pkgstate::state_target_binding_identity::canonical_domain(),
      pkgstate::installed_state_snapshot_identity::canonical_domain(),
      pkgstate::state_publication_request_identity::canonical_domain(),
      pkgstate::state_publication_receipt_identity::canonical_domain(),
      pkgstate::legacy_import_request_identity::canonical_domain(),
      pkgstate::legacy_import_receipt_identity::canonical_domain(),
      pkgstate::legacy_package_observation_identity::canonical_domain(),
      pkgstate::legacy_snapshot_observation_identity::canonical_domain(),
  };
  for (std::size_t left = 0; left < domains.size(); ++left)
  {
    for (std::size_t right = left + 1; right < domains.size(); ++right)
      CHECK(domains[left] != domains[right]);
  }

  const std::string valid = "v1:sha256:" + zeros();
  CHECK(pkgstate::installed_control_identity::parse(valid).string() == valid);

  check_digest_error(pkgstate::digest_error_code::invalid_format, [] {
    static_cast<void>(pkgstate::installed_control_identity::parse("v1:sha256"));
  });
  check_digest_error(pkgstate::digest_error_code::invalid_format, [&] {
    static_cast<void>(
        pkgstate::installed_control_identity::parse("v1:sha256:" + zeros() + ":x"));
  });
  check_digest_error(pkgstate::digest_error_code::unsupported_version, [&] {
    static_cast<void>(
        pkgstate::installed_control_identity::parse("v2:sha256:" + zeros()));
  });
  check_digest_error(pkgstate::digest_error_code::unsupported_algorithm, [&] {
    static_cast<void>(
        pkgstate::installed_control_identity::parse("v1:sha512:" + zeros()));
  });
  check_digest_error(pkgstate::digest_error_code::invalid_length, [&] {
    static_cast<void>(pkgstate::installed_control_identity::parse(
        "v1:sha256:" + std::string(63, '0')));
  });
  check_digest_error(pkgstate::digest_error_code::invalid_hex, [&] {
    std::string uppercase = zeros();
    uppercase.back() = 'A';
    static_cast<void>(pkgstate::installed_control_identity::parse(
        "v1:sha256:" + uppercase));
  });
  check_digest_error(pkgstate::digest_error_code::invalid_hex, [&] {
    std::string invalid = zeros();
    invalid.back() = 'g';
    static_cast<void>(pkgstate::installed_control_identity::parse(
        "v1:sha256:" + invalid));
  });

  return 0;
}
