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

pkgstate::installed_control alpha_control(
    const pkgstate::package_release& release)
{
  pkgstate::installed_control_completeness completeness;
  completeness.runtime_dependencies =
      pkgstate::installed_control_fact_state::recorded_at_installation;
  completeness.removal_lifecycle =
      pkgstate::installed_control_fact_state::recorded_in_compatibility_storage;
  completeness.target_profile =
      pkgstate::installed_control_fact_state::supplied_by_migration;
  completeness.provenance =
      pkgstate::installed_control_fact_state::recorded_at_installation;

  return pkgstate::installed_control::make(
      release,
      completeness,
      {
          pkgstate::runtime_dependency_declaration::make("libc >= 2.39"),
          pkgstate::runtime_dependency_declaration::make("zlib >= 1.3"),
      },
      {
          pkgstate::removal_lifecycle_declaration::make(
              pkgstate::removal_lifecycle_phase::post_remove,
              "application/x-sh",
              "echo post\n"),
          pkgstate::removal_lifecycle_declaration::make(
              pkgstate::removal_lifecycle_phase::pre_remove,
              "application/x-sh",
              std::string("echo pre\0tail", 13)),
      },
      {
          pkgstate::target_profile_fact::make("abi", "gnu"),
          pkgstate::target_profile_fact::make("arch", "x86_64"),
      },
      {
          pkgstate::control_provenance::make(
              pkgstate::control_provenance_kind::candidate_control,
              id<pkgstate::installed_control_identity>(0x91).string()),
      });
}

pkgstate::installed_control beta_control(
    const pkgstate::package_release& release)
{
  pkgstate::installed_control_completeness completeness;
  completeness.runtime_dependencies =
      pkgstate::installed_control_fact_state::historically_unavailable;
  completeness.removal_lifecycle =
      pkgstate::installed_control_fact_state::recorded_at_installation;
  completeness.target_profile =
      pkgstate::installed_control_fact_state::historically_unavailable;
  completeness.provenance =
      pkgstate::installed_control_fact_state::historically_unavailable;

  return pkgstate::installed_control::make(
      release, completeness, {}, {}, {}, {});
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
      alpha_control(alpha_release),
      target,
      {
          entry("usr/bin/alpha", pkgstate::owned_entry_type::non_directory),
          entry("usr/share/common", pkgstate::owned_entry_type::directory),
      });
  const auto beta = pkgstate::installed_package::make(
      beta_release,
      beta_control(beta_release),
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

    const auto& state_control = state_package.control();
    const auto& plan_control = plan_package.control_projection();
    const auto& state_completeness = state_control.completeness();
    const auto& plan_completeness = plan_control.completeness();

    CHECK(pkgplan::is_known(plan_completeness.runtime_dependencies) ==
          pkgstate::is_known(state_completeness.runtime_dependencies));
    CHECK(pkgplan::is_known(plan_completeness.removal_lifecycle) ==
          pkgstate::is_known(state_completeness.removal_lifecycle));
    CHECK(pkgplan::is_known(plan_completeness.target_profile) ==
          pkgstate::is_known(state_completeness.target_profile));

    CHECK(plan_control.runtime_dependencies().size() ==
          state_control.runtime_dependencies().size());
    for (std::size_t fact = 0;
         fact < state_control.runtime_dependencies().size();
         ++fact)
    {
      CHECK(plan_control.runtime_dependencies()[fact].expression() ==
            state_control.runtime_dependencies()[fact].expression());
    }

    CHECK(plan_control.removal_lifecycle().size() ==
          state_control.removal_lifecycle().size());
    for (std::size_t fact = 0;
         fact < state_control.removal_lifecycle().size();
         ++fact)
    {
      const auto& state_declaration =
          state_control.removal_lifecycle()[fact];
      const auto& plan_declaration =
          plan_control.removal_lifecycle()[fact];
      CHECK(static_cast<unsigned>(plan_declaration.phase()) ==
            static_cast<unsigned>(state_declaration.phase()));
      CHECK(plan_declaration.format() == state_declaration.format());
      CHECK(plan_declaration.material() == state_declaration.material());
    }

    CHECK(plan_control.target_profile().size() ==
          state_control.target_profile().size());
    for (std::size_t fact = 0;
         fact < state_control.target_profile().size();
         ++fact)
    {
      CHECK(plan_control.target_profile()[fact].name() ==
            state_control.target_profile()[fact].name());
      CHECK(plan_control.target_profile()[fact].value() ==
            state_control.target_profile()[fact].value());
    }
  }

  const auto& alpha_state_control = source.packages()[0].control();
  const auto& alpha_plan_control =
      projected.packages()[0].control_projection();
  CHECK(alpha_state_control.provenance().size() == 1);
  CHECK(alpha_plan_control.completeness().runtime_dependencies ==
        pkgplan::control_fact_availability::known);
  CHECK(alpha_plan_control.completeness().removal_lifecycle ==
        pkgplan::control_fact_availability::known);
  CHECK(alpha_plan_control.completeness().target_profile ==
        pkgplan::control_fact_availability::known);

  const auto& beta_plan_control =
      projected.packages()[1].control_projection();
  CHECK(beta_plan_control.completeness().runtime_dependencies ==
        pkgplan::control_fact_availability::historically_unavailable);
  CHECK(beta_plan_control.completeness().removal_lifecycle ==
        pkgplan::control_fact_availability::known);
  CHECK(beta_plan_control.completeness().target_profile ==
        pkgplan::control_fact_availability::historically_unavailable);
  CHECK(beta_plan_control.runtime_dependencies().empty());
  CHECK(beta_plan_control.removal_lifecycle().empty());
  CHECK(beta_plan_control.target_profile().empty());

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
