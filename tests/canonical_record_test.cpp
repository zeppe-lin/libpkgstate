// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "canonical_record.h"

#include <array>
#include <cstdint>
#include <string>

#include <libpkgstate/error.h>

#include "test.h"

namespace {

std::string
hexadecimal(const pkgstate::sha256_digest_bytes& bytes)
{
  constexpr char digits[] = "0123456789abcdef";
  std::string result;
  result.reserve(bytes.size() * 2);
  for (const std::uint8_t byte : bytes)
  {
    result.push_back(digits[(byte >> 4U) & 0x0fU]);
    result.push_back(digits[byte & 0x0fU]);
  }
  return result;
}

} // namespace

int
main()
{
  pkgstate::sha256_digest_bytes subordinate{};
  for (std::size_t index = 0; index < subordinate.size(); ++index)
    subordinate[index] = static_cast<std::uint8_t>(index);

  pkgstate::detail::canonical_record vector("pkgstate/test-vector/1");
  vector.append_bool(true);
  vector.append_u8(0x7f);
  vector.append_u16(0x0123);
  vector.append_u32(0x456789ab);
  vector.append_u64(UINT64_C(0xcdef0123456789ab));
  vector.append_bytes("alpha");
  vector.append_bytes("");
  vector.append_digest(
      pkgstate::package_release_identity::from_sha256(subordinate));

  CHECK(vector.bytes().size() == 122);
  CHECK(hexadecimal(vector.sha256()) ==
        "5c7e23244aa27557feec3db2827ec676d"
        "34720247cd9cb5e0b3d8367bc1b7dfa");

  pkgstate::detail::canonical_record first_domain("pkgstate/domain-a/1");
  pkgstate::detail::canonical_record second_domain("pkgstate/domain-b/1");
  first_domain.append_bytes("same");
  second_domain.append_bytes("same");
  CHECK(first_domain.sha256() != second_domain.sha256());

  pkgstate::detail::canonical_record first_fields("pkgstate/fields/1");
  pkgstate::detail::canonical_record second_fields("pkgstate/fields/1");
  first_fields.append_bytes("ab");
  first_fields.append_bytes("c");
  second_fields.append_bytes("a");
  second_fields.append_bytes("bc");
  CHECK(first_fields.sha256() != second_fields.sha256());

  check_throws<pkgstate::state_error>([] {
    static_cast<void>(pkgstate::detail::canonical_record(""));
  });
  check_throws<pkgstate::state_error>([] {
    static_cast<void>(pkgstate::detail::canonical_record("bad\ndomain"));
  });

  return 0;
}
