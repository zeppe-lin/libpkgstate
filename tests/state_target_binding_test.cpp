// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/state_target_binding.h>

#include <type_traits>

#include "test.h"

namespace {

constexpr const char* zero_digest =
    "v1:sha256:00000000000000000000000000000000"
    "00000000000000000000000000000000";
constexpr const char* one_digest =
    "v1:sha256:11111111111111111111111111111111"
    "11111111111111111111111111111111";
constexpr const char* two_digest =
    "v1:sha256:22222222222222222222222222222222"
    "22222222222222222222222222222222";
constexpr const char* three_digest =
    "v1:sha256:33333333333333333333333333333333"
    "33333333333333333333333333333333";
constexpr const char* four_digest =
    "v1:sha256:44444444444444444444444444444444"
    "44444444444444444444444444444444";

pkgstate::state_target_binding
binding(const char* publication = four_digest)
{
  return pkgstate::state_target_binding::make(
      pkgstate::managed_target_identity::parse(zero_digest),
      pkgstate::state_store_identity::parse(one_digest),
      pkgstate::root_view_identity::parse(two_digest),
      pkgstate::state_backend_identity::parse(three_digest),
      pkgstate::publication_domain_identity::parse(publication));
}

} // namespace

int
main()
{
  static_assert(!std::is_same_v<pkgstate::managed_target_identity,
                                pkgstate::state_store_identity>);
  static_assert(!std::is_same_v<pkgstate::root_view_identity,
                                pkgstate::state_backend_identity>);
  static_assert(!std::is_same_v<pkgstate::publication_domain_identity,
                                pkgstate::state_target_binding_identity>);

  const pkgstate::state_target_binding target = binding();
  CHECK(target.managed_target().string() == zero_digest);
  CHECK(target.state_store().string() == one_digest);
  CHECK(target.root_view().string() == two_digest);
  CHECK(target.state_backend().string() == three_digest);
  CHECK(target.publication_domain().string() == four_digest);
  CHECK(target.identity().string() ==
        "v1:sha256:31ad5e9313cc0fc611acd1b374b0159"
        "40945ebe6768f9782ef9f298de3765bb0");

  CHECK(target == binding());
  CHECK(!(target != binding()));

  const pkgstate::state_target_binding changed = binding(three_digest);
  CHECK(target != changed);
  CHECK(changed < target);

  const pkgstate::state_target_binding same_bytes =
      pkgstate::state_target_binding::make(
          pkgstate::managed_target_identity::parse(zero_digest),
          pkgstate::state_store_identity::parse(zero_digest),
          pkgstate::root_view_identity::parse(zero_digest),
          pkgstate::state_backend_identity::parse(zero_digest),
          pkgstate::publication_domain_identity::parse(zero_digest));
  CHECK(same_bytes.managed_target().bytes() ==
        same_bytes.state_store().bytes());
  CHECK(same_bytes.state_store().bytes() == same_bytes.root_view().bytes());

  return 0;
}
