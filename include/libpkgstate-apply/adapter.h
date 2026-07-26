// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file adapter.h
 * \brief Projection of completed package application into installed state.
 */

#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include <libpkgapply/request.h>
#include <libpkgapply/result.h>
#include <libpkgapply/state_projection.h>
#include <libpkgstate/publication_request.h>
#include <libpkgstate/snapshot.h>

namespace pkgstate::apply_adapter {

/*! \brief Structured reason that application-to-state projection failed. */
enum class projection_error_code : std::uint8_t {
  request_binding_mismatch = 1,   //!< Evidence names another request or control.
  operation_binding_mismatch = 2, //!< Operation classes disagree.
  plan_binding_mismatch = 3,      //!< Request, evidence, and plan disagree.
  target_binding_mismatch = 4,    //!< Application and state targets disagree.
  state_projection_mismatch = 5,  //!< Evidence names another lease projection.
  expected_state_mismatch = 6,    //!< Projection does not name the snapshot.
  ownership_projection_mismatch = 7, //!< Projected owners differ from state.
  package_state_mismatch = 8,     //!< Required old package state is absent.
  completed_path_mismatch = 9,    //!< Completed ownership truth is unsuitable.
  identity_translation = 10,      //!< External identity representation failed.
  path_translation = 11,          //!< Canonical path vocabulary disagreed.
  control_translation = 12,       //!< Incoming control vocabulary disagreed.
  publication_construction = 13,  //!< Canonical publication request rejected.
};

/*! \brief Failure to project application authority into state vocabulary. */
class projection_error final : public std::invalid_argument {
public:
  /*! \brief Construct one typed projection failure. */
  projection_error(projection_error_code code, std::string message);

  /*! \brief Return the machine-readable failure class. */
  [[nodiscard]] projection_error_code code() const noexcept;

private:
  projection_error_code code_;
};

/*!
 * \brief Construct one canonical publication request from completed application.
 *
 * The function verifies the exact request, plan, target, execution-control,
 * lease-projection, expected-snapshot, ownership, and completed-path bindings.
 * It performs no store I/O and never publishes state.
 *
 * \throws projection_error when any authority or vocabulary disagrees.
 */
[[nodiscard]] state_publication_request
project_completed_application(
    const snapshot& expected_state,
    const pkgapply::lease_bound_state_projection& application_state,
    const pkgapply::package_application_request& request,
    const pkgapply::completed_application_evidence& evidence);

} // namespace pkgstate::apply_adapter
