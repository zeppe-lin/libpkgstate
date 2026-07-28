// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file adapter.h
 *  \brief Projection of completed package application into installed state.
 */
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include <libpkgapply/request.h>
#include <libpkgapply/result.h>
#include <libpkgapply/state_projection.h>
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

/*! \brief Construct installation publication from completed native application.
 *
 * The request itself retains the exact successful libpkgbuild result and
 * independently inspected libpkgimage authority admitted by libpkgapply.
 * This adapter projects those values through libpkgstate-source and
 * libpkgstate-build, then binds the completed filesystem evidence and the
 * caller-selected initial installation reason. No caller-supplied build or
 * source authority is accepted separately.
 */
[[nodiscard]] state_publication_request project_completed_application(
    const snapshot& expected_state,
    const pkgapply::lease_bound_state_projection& application_state,
    const pkgapply::installation_application_request& request,
    const pkgapply::completed_application_evidence& evidence,
    installation_reason reason);

/*! \brief Construct installation publication with exact transaction evidence. */
[[nodiscard]] state_publication_request project_completed_application(
    const snapshot& expected_state,
    const pkgapply::lease_bound_state_projection& application_state,
    const pkgapply::installation_application_request& request,
    const pkgapply::completed_application_evidence& evidence,
    installation_reason reason,
    transaction_evidence_identity transaction_evidence);

/*! \brief Construct replacement publication from completed native application.
 *
 * The existing installed reason is retained. Incoming source and build
 * authority are taken only from the exact request named by completed evidence.
 */
[[nodiscard]] state_publication_request project_completed_application(
    const snapshot& expected_state,
    const pkgapply::lease_bound_state_projection& application_state,
    const pkgapply::upgrade_application_request& request,
    const pkgapply::completed_application_evidence& evidence);

/*! \brief Construct replacement publication with exact transaction evidence. */
[[nodiscard]] state_publication_request project_completed_application(
    const snapshot& expected_state,
    const pkgapply::lease_bound_state_projection& application_state,
    const pkgapply::upgrade_application_request& request,
    const pkgapply::completed_application_evidence& evidence,
    transaction_evidence_identity transaction_evidence);

/*! \brief Construct removal publication from completed native application. */
[[nodiscard]] state_publication_request project_completed_application(
    const snapshot& expected_state,
    const pkgapply::lease_bound_state_projection& application_state,
    const pkgapply::removal_application_request& request,
    const pkgapply::completed_application_evidence& evidence);

/*! \brief Construct removal publication with exact transaction evidence. */
[[nodiscard]] state_publication_request project_completed_application(
    const snapshot& expected_state,
    const pkgapply::lease_bound_state_projection& application_state,
    const pkgapply::removal_application_request& request,
    const pkgapply::completed_application_evidence& evidence,
    transaction_evidence_identity transaction_evidence);

} // namespace pkgstate::apply_adapter
