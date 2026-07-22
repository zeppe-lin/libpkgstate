// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate-plan/adapter.h>

#include <libpkgstate/legacy_snapshot.h>

#include <type_traits>
#include <utility>
#include <vector>

#include "test.h"

namespace {

template<typename Identity>
Identity id(unsigned char byte)
{
  pkgstate::sha256_digest_bytes bytes{};
  bytes.fill(byte);
  return Identity::from_sha256(bytes);
}

pkgstate::state_target_binding binding(unsigned char store_byte)
{
  return pkgstate::state_target_binding::make(
      id<pkgstate::managed_target_identity>(0x01),
      id<pkgstate::state_store_identity>(store_byte),
      id<pkgstate::root_view_identity>(0x03),
      id<pkgstate::state_backend_identity>(0x04),
      id<pkgstate::publication_domain_identity>(0x05));
}

pkgstate::installed_control control(const pkgstate::package_release& release)
{
  return pkgstate::installed_control::make(
      release,
      {},
      {},
      {},
      {},
      {});
}

pkgstate::owned_entry entry(const char* path,
                            pkgstate::owned_entry_type type)
{
  return {pkgstate::package_path::parse(path), type};
}

pkgstate::snapshot state()
{
  const auto target = binding(0x02);
  const auto alpha_release =
      pkgstate::package_release::make("alpha", "1.0", "2");
  const auto beta_release =
      pkgstate::package_release::make("beta", "3.1", "1");

  const auto alpha = pkgstate::installed_package::make(
      alpha_release,
      control(alpha_release),
      target,
      {
          entry("usr/bin/alpha", pkgstate::owned_entry_type::non_directory),
          entry("usr/share/common", pkgstate::owned_entry_type::directory),
      });
  const auto beta = pkgstate::installed_package::make(
      beta_release,
      control(beta_release),
      target,
      {
          entry("usr/bin/beta", pkgstate::owned_entry_type::non_directory),
          entry("usr/share/common", pkgstate::owned_entry_type::directory),
      });

  return pkgstate::snapshot::make(target, {beta, alpha});
}

} // namespace

int
main()
{
  using project_function =
      pkgstate::plan_adapter::installed_state_projection (*)(
          const pkgstate::snapshot&,
          const pkgstate::plan_adapter::planning_target_context&);
  static_assert(!std::is_invocable_v<
                project_function,
                const pkgstate::legacy_snapshot&,
                const pkgstate::plan_adapter::planning_target_context&>);

  const auto source = state();
  const auto planner_target =
      id<pkgplan::target_system_context_identity>(0x71);
  const pkgstate::plan_adapter::planning_target_context target(
      planner_target, source.target_binding());

  const auto projected =
      pkgstate::plan_adapter::project_installed_state(source, target);

  CHECK(projected.target() == planner_target);
  CHECK(projected.packages().size() == 2);
  CHECK(projected.packages()[0].release().name() == "alpha");
  CHECK(projected.packages()[1].release().name() == "beta");

  for (std::size_t index = 0; index < source.packages().size(); ++index)
  {
    const auto& state_package = source.packages()[index];
    const auto& plan_package = projected.packages()[index];
    CHECK(plan_package.identity().string() ==
          state_package.identity().string());
    CHECK(plan_package.control().string() ==
          state_package.control().identity().string());
    CHECK(plan_package.snapshot().string() == source.identity().string());
    CHECK(plan_package.release().identity().string() ==
          state_package.release().identity().string());
    CHECK(plan_package.release().name() == state_package.release().name());
    CHECK(plan_package.release().version() ==
          state_package.release().version());
    CHECK(plan_package.release().release() ==
          state_package.release().release());
  }

  CHECK(projected.ownership().identity().string() ==
        source.ownership_identity().string());
  CHECK(projected.ownership().snapshot().string() ==
        source.identity().string());
  CHECK(projected.ownership().completeness() ==
        pkgplan::fact_set_completeness::complete);
  CHECK(projected.ownership().claims().size() == 4);

  const auto common = pkgplan::package_path::parse("usr/share/common");
  const auto common_owners = projected.ownership().owners(common);
  CHECK(common_owners.size() == 2);
  for (const auto& claim : projected.ownership().claims())
  {
    CHECK(!claim.recorded_object().has_value());
  }

  const pkgstate::plan_adapter::planning_target_context wrong_target(
      planner_target, binding(0x22));
  try
  {
    static_cast<void>(
        pkgstate::plan_adapter::project_installed_state(source, wrong_target));
    CHECK(false);
  }
  catch (const pkgstate::plan_adapter::projection_error& error)
  {
    CHECK(error.code() ==
          pkgstate::plan_adapter::projection_error_code::
              target_binding_mismatch);
  }

  return 0;
}
