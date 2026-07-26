// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate-plan/adapter.h>
#include <libpkgstate/libpkgstate.h>

#include <string>

namespace {

template<typename Identity>
Identity
identity(char digit)
{
  return Identity::parse("v1:sha256:" + std::string(64, digit));
}

} // namespace

int
main()
{
  const pkgstate::state_target_binding target =
      pkgstate::state_target_binding::make(
          identity<pkgstate::managed_target_identity>('1'),
          identity<pkgstate::state_store_identity>('2'),
          identity<pkgstate::root_view_identity>('3'),
          identity<pkgstate::state_backend_identity>('4'),
          identity<pkgstate::publication_domain_identity>('5'));
  const pkgstate::snapshot state = pkgstate::snapshot::make(target, {});
  const pkgstate::plan_adapter::planning_target_context planning_target(
      identity<pkgplan::target_system_context_identity>('6'), target);
  const auto projection =
      pkgstate::plan_adapter::project_installed_state(state, planning_target);

  return projection.target().string() == planning_target.identity().string()
             && projection.packages().empty()
             && projection.ownership().claims().empty()
         ? 0
         : 1;
}
