// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "native_fixture.h"
#include "test.h"

#include <cstdint>
#include <string>

#include <libpkgplan/control.h>
#include <libpkgstate-plan/adapter.h>

namespace {

template<typename Identity>
Identity plan_identity(std::uint8_t seed)
{
  pkgplan::sha256_digest_bytes bytes{};
  for (std::size_t index = 0; index < bytes.size(); ++index)
    bytes[index] = static_cast<std::uint8_t>(seed + index);
  return Identity::from_sha256(bytes);
}

void projects_native_installed_state()
{
  const pkgstate::snapshot source =
      native_fixture::state_with_package("example", 20);
  const auto planner_target =
      plan_identity<pkgplan::target_system_context_identity>(70);
  const pkgstate::plan_adapter::planning_target_context target(
      planner_target, source.target_binding());

  const pkgstate::plan_adapter::installed_state_projection projected =
      pkgstate::plan_adapter::project_installed_state(source, target);

  TEST_EQ(projected.target(), planner_target);
  TEST_EQ(projected.packages().size(), std::size_t{1});
  const pkgstate::installed_package& state_package = source.packages().front();
  const pkgplan::installed_package_fact& package = projected.packages().front();
  TEST_EQ(package.identity().string(), state_package.identity().string());
  TEST_EQ(package.control().string(), state_package.control().identity().string());
  TEST_EQ(package.snapshot().string(), source.identity().string());
  TEST_EQ(package.release().identity().string(),
          state_package.release().identity().string());
  TEST_EQ(package.release().name(), "example");
  TEST_EQ(package.release().version(), "1.2.3");
  TEST_EQ(package.release().release(), "1");

  const pkgplan::installed_control_projection& control =
      package.control_projection();
  TEST_EQ(control.completeness().runtime_dependencies,
          pkgplan::control_fact_availability::known);
  TEST_EQ(control.completeness().removal_lifecycle,
          pkgplan::control_fact_availability::known);
  TEST_EQ(control.completeness().target_profile,
          pkgplan::control_fact_availability::known);
  TEST_EQ(control.runtime_dependencies().size(), std::size_t{1});
  TEST_EQ(control.runtime_dependencies().front().expression(), "libfoo");
  TEST_EQ(control.removal_lifecycle().size(), std::size_t{1});
  TEST_EQ(control.removal_lifecycle().front().phase(),
          pkgplan::removal_lifecycle_phase::pre_remove);
  TEST_EQ(control.removal_lifecycle().front().format(), "text/x-posix-shell");
  TEST_EQ(control.removal_lifecycle().front().material(), "echo removing\n");
  TEST_EQ(control.target_profile().size(), std::size_t{1});
  TEST_EQ(control.target_profile().front().name(),
          "pkgsource.target-architectures");
  TEST_EQ(control.target_profile().front().value(), "x86_64");

  TEST_EQ(projected.ownership().identity().string(),
          source.ownership_identity().string());
  TEST_EQ(projected.ownership().snapshot().string(), source.identity().string());
  TEST_EQ(projected.ownership().completeness(),
          pkgplan::fact_set_completeness::complete);
  TEST_EQ(projected.ownership().claims().size(), std::size_t{2});
  for (const pkgplan::installed_ownership_claim& claim :
       projected.ownership().claims())
  {
    TEST(claim.recorded_object().has_value());
    const pkgplan::filesystem_object_metadata& object =
        *claim.recorded_object();
    TEST_EQ(object.kind(), pkgplan::filesystem_object_kind::regular);
    TEST(object.size().has_value());
    TEST_EQ(*object.size(), std::uint64_t{7});
    TEST(object.mtime().has_value());
    TEST(object.regular_content().has_value());
  }
  TEST_EQ(projected.ownership().claims().back().recorded_object()->mode(),
          std::uint32_t{0755});
  TEST_EQ(
      projected.ownership().claims().back().recorded_object()->mtime()->seconds(),
      std::int64_t{-1});
}

void rejects_target_binding_mismatch()
{
  const pkgstate::snapshot source = native_fixture::state_with_package();
  const pkgstate::plan_adapter::planning_target_context target(
      plan_identity<pkgplan::target_system_context_identity>(70),
      native_fixture::target(40));
  try
  {
    (void)pkgstate::plan_adapter::project_installed_state(source, target);
    TEST(false);
  }
  catch (const pkgstate::plan_adapter::projection_error& error)
  {
    TEST_EQ(error.code(),
            pkgstate::plan_adapter::projection_error_code::
                target_binding_mismatch);
  }
}

} // namespace

int main()
{
  projects_native_installed_state();
  rejects_target_binding_mismatch();
}
