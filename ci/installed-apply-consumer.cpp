// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate-apply/adapter.h>
#include <libpkgstate-apply/state_projection.h>

#include <string>

int
main()
{
  const auto reader = &pkgstate::apply_adapter::read_application_state;
  static_cast<void>(reader);
  const pkgstate::apply_adapter::projection_error projection(
      pkgstate::apply_adapter::projection_error_code::request_binding_mismatch,
      "consumer probe");
  const pkgstate::apply_adapter::application_state_projection_error state(
      pkgstate::apply_adapter::application_state_projection_error_code::
          lease_not_held,
      "state consumer probe");
  return projection.code() ==
                 pkgstate::apply_adapter::projection_error_code::
                     request_binding_mismatch &&
             std::string(projection.what()) == "consumer probe" &&
             state.code() ==
                 pkgstate::apply_adapter::application_state_projection_error_code::
                     lease_not_held &&
             std::string(state.what()) == "state consumer probe" &&
             pkgstate::apply_adapter::
                     application_state_projection_evidence_schema_version == 1
         ? 0
         : 1;
}
