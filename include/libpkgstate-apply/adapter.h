// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file adapter.h
 *  \brief Projection of completed package application into installed state.
 */
#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

#include <libpkgapply/request.h>
#include <libpkgapply/result.h>
#include <libpkgapply/state_projection.h>
#include <libpkgstate-build/adapter.h>
#include <libpkgstate/installed_control.h>
#include <libpkgstate/publication_request.h>
#include <libpkgstate/snapshot.h>

namespace pkgstate::apply_adapter {

enum class projection_error_code : std::uint8_t {
  request_binding_mismatch = 1,
  operation_binding_mismatch = 2,
  plan_binding_mismatch = 3,
  target_binding_mismatch = 4,
  state_projection_mismatch = 5,
  expected_state_mismatch = 6,
  ownership_projection_mismatch = 7,
  package_state_mismatch = 8,
  completed_path_mismatch = 9,
  identity_translation = 10,
  path_translation = 11,
  incoming_authority_mismatch = 12,
  publication_construction = 13,
};

class projection_error final : public std::invalid_argument {
public:
  projection_error(projection_error_code code, std::string message);
  [[nodiscard]] projection_error_code code() const noexcept;
private:
  projection_error_code code_;
};

enum class incoming_authority_kind : std::uint8_t {
  initial_install = 1,
  replacement = 2,
};

/*! \brief Non-application authority needed to construct native state.
 *
 * libpkgapply proves completed target effects. It does not own the sealed
 * package-source record, installation reason, build-input set, or build result.
 * This value supplies those facts explicitly without teaching libpkgapply or
 * libpkgstate to reconstruct them from planner control or artifact names.
 */
class incoming_installation_authority final {
public:
  [[nodiscard]] static incoming_installation_authority install(
      build_adapter::build_authority build,
      installation_reason reason);

  [[nodiscard]] static incoming_installation_authority replacement(
      build_adapter::build_authority build);

  [[nodiscard]] incoming_authority_kind kind() const noexcept;
  [[nodiscard]] const package_source_record& source() const noexcept;
  [[nodiscard]] const build_provenance& build() const noexcept;
  [[nodiscard]] const std::optional<installation_reason>& reason() const noexcept;

private:
  incoming_installation_authority(
      incoming_authority_kind kind,
      build_adapter::build_authority build,
      std::optional<installation_reason> reason);

  incoming_authority_kind kind_;
  build_adapter::build_authority build_;
  std::optional<installation_reason> reason_;
};

/*! \brief Construct one canonical publication request from completed application.
 *
 * Installation and upgrade require the matching incoming authority. Removal
 * requires its absence. The function verifies request, plan, target, lease,
 * expected snapshot, package control, source release, ownership, and completed
 * path bindings. It performs no store I/O and never publishes state.
 */
[[nodiscard]] state_publication_request project_completed_application(
    const snapshot& expected_state,
    const pkgapply::lease_bound_state_projection& application_state,
    const pkgapply::package_application_request& request,
    const pkgapply::completed_application_evidence& evidence,
    std::optional<incoming_installation_authority> incoming = std::nullopt);

} // namespace pkgstate::apply_adapter
