// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/libpkgstate.h>

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "test.h"

namespace {

template<typename Identity>
Identity
identity(char digit)
{
  return Identity::parse("v1:sha256:" + std::string(64, digit));
}

pkgstate::state_target_binding
binding(char first)
{
  return pkgstate::state_target_binding::make(
      identity<pkgstate::managed_target_identity>(first),
      identity<pkgstate::state_store_identity>(
          static_cast<char>(first + 1)),
      identity<pkgstate::root_view_identity>(
          static_cast<char>(first + 2)),
      identity<pkgstate::state_backend_identity>(
          static_cast<char>(first + 3)),
      identity<pkgstate::publication_domain_identity>(
          static_cast<char>(first + 4)));
}

pkgstate::package_path
path(const char* value)
{
  return pkgstate::package_path::parse(value);
}

pkgstate::owned_entry
file(const char* value)
{
  return {path(value), pkgstate::owned_entry_type::non_directory};
}

pkgstate::owned_entry
directory(const char* value)
{
  return {path(value), pkgstate::owned_entry_type::directory};
}

pkgstate::installed_package
package(const char* name,
        const char* version,
        const char* distribution_release,
        const pkgstate::state_target_binding& target,
        std::vector<pkgstate::owned_entry> manifest)
{
  const pkgstate::package_release release =
      pkgstate::package_release::make(
          name, version, distribution_release);
  const pkgstate::installed_control control =
      pkgstate::installed_control::make(release, {}, {}, {}, {}, {});
  return pkgstate::installed_package::make(
      release, control, target, std::move(manifest));
}

} // namespace

int
main()
{
  static_assert(!std::is_same_v<
                pkgstate::ownership_inventory_identity,
                pkgstate::installed_state_snapshot_identity>);

  const pkgstate::state_target_binding target = binding('1');
  const pkgstate::snapshot empty = pkgstate::snapshot::make(target);

  CHECK(empty.ownership_identity().string() ==
        "v1:sha256:"
        "b7d9e71bd486cab9588465d14b141cff65bbc4fe801a4f34814a0990b4a87c24");
  CHECK(empty.identity().string() ==
        "v1:sha256:"
        "6d3334ad6a4633f123b62f79aa144b75720aeceed1ddd92c92f2ac834eef2c46");

  const pkgstate::snapshot state = pkgstate::snapshot::make(target, {
      package("zlib", "1.3", "1", target,
              {file("usr/lib/libz.so")}),
      package("base", "1.0", "1", target,
              {file("usr/bin/base"), directory("usr/share/common")}),
      package("other", "2", "1", target,
              {directory("usr/share/common")}),
  });

  CHECK(state.ownership_identity().string() ==
        "v1:sha256:"
        "1593ee13dbdbad279c160b02461e278a0b671ce163b277a08fa6e634a294b11e");
  CHECK(state.identity().string() ==
        "v1:sha256:"
        "cdf46175575890334ff0f63b01a93b2ae6d4083d732c91ff8f157732d5e94253");

  const pkgstate::snapshot permuted = pkgstate::snapshot::make(target, {
      package("other", "2", "1", target,
              {directory("usr/share/common")}),
      package("base", "1.0", "1", target,
              {directory("usr/share/common"), file("usr/bin/base")}),
      package("zlib", "1.3", "1", target,
              {file("usr/lib/libz.so")}),
  });

  CHECK(permuted.ownership_identity() == state.ownership_identity());
  CHECK(permuted.identity() == state.identity());

  const pkgstate::package_release base_release =
      pkgstate::package_release::make("base", "1.0", "1");
  const pkgstate::installed_control changed_control =
      pkgstate::installed_control::make(
          base_release, {},
          {pkgstate::runtime_dependency_declaration::make("libc")},
          {}, {}, {});
  const pkgstate::installed_package changed_base =
      pkgstate::installed_package::make(
          base_release, changed_control, target,
          {file("usr/bin/base"), directory("usr/share/common")});
  const pkgstate::snapshot changed_owner = pkgstate::snapshot::make(target, {
      changed_base,
      package("other", "2", "1", target,
              {directory("usr/share/common")}),
      package("zlib", "1.3", "1", target,
              {file("usr/lib/libz.so")}),
  });

  CHECK(changed_owner.ownership_identity() != state.ownership_identity());
  CHECK(changed_owner.identity() != state.identity());

  const pkgstate::snapshot changed_relation = pkgstate::snapshot::make(target, {
      package("base", "1.0", "1", target,
              {file("usr/bin/base"), directory("usr/share/common")}),
      package("other", "2", "1", target, {}),
      package("zlib", "1.3", "1", target,
              {file("usr/lib/libz.so")}),
  });

  CHECK(changed_relation.ownership_identity() != state.ownership_identity());
  CHECK(changed_relation.identity() != state.identity());

  return 0;
}
