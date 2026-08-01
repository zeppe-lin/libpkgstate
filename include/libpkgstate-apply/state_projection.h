// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file state_projection.h
 *  \brief Lease-bound projection of canonical installed state into libpkgapply.
 */
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include <libpkgapply/mutation_lease.h>
#include <libpkgapply/request.h>
#include <libpkgapply/state_projection.h>
#include <libpkgstate/canonical_store.h>
#include <libpkgstate/snapshot.h>

namespace pkgstate::apply_adapter {

/*! \brief Canonical evidence schema for one lease-bound state read. */
inline constexpr std::uint16_t application_state_projection_evidence_schema_version = 1;

/*! \brief Structured refusal while projecting current state for application. */
enum class application_state_projection_error_code : std::uint8_t {
  request_binding_mismatch = 1,
  lease_not_held = 2,
  lease_lost = 3,
  lease_target_mismatch = 4,
  lease_domain_mismatch = 5,
  target_binding_mismatch = 6,
  expected_snapshot_mismatch = 7,
  ownership_inventory_mismatch = 8,
  path_owners_mismatch = 9,
  identity_translation = 10,
  path_translation = 11,
  evidence_construction = 12,
};

/*! \brief Invalid authority universe for a lease-bound application-state read. */
class application_state_projection_error final : public std::invalid_argument {
public:
  application_state_projection_error(
      application_state_projection_error_code code,
      std::string message);

  [[nodiscard]] application_state_projection_error_code code() const noexcept;

private:
  application_state_projection_error_code code_;
};

/*! \brief One exact canonical snapshot and its application path projection.
 *
 * The value owns both objects so the caller cannot accidentally pair a
 * projection with another state epoch when constructing an effect driver.
 */
class lease_bound_application_state final {
public:
  [[nodiscard]] const snapshot& state() const noexcept;
  [[nodiscard]] const pkgapply::lease_bound_state_projection&
  projection() const noexcept;

private:
  friend lease_bound_application_state read_application_state(
      const pkgapply::package_application_request&,
      const pkgapply::target_mutation_lease&,
      const canonical_store&);

  lease_bound_application_state(
      snapshot state,
      pkgapply::lease_bound_state_projection projection);

  snapshot state_;
  pkgapply::lease_bound_state_projection projection_;
};

/*! \brief Read canonical state while one caller-owned target lease is held.
 *
 * The function performs exactly one store read, validates that the returned
 * state is the epoch admitted by the application request, projects the exact
 * plan path-owner universe, and binds deterministic projection evidence to the
 * live lease acquisition. It does not acquire a lease, initialize a store,
 * observe the target filesystem, publish state, or perform application.
 *
 * \throws application_state_projection_error when immutable authorities differ.
 * \throws store_error when the canonical store cannot be read.
 */
[[nodiscard]] lease_bound_application_state read_application_state(
    const pkgapply::package_application_request& request,
    const pkgapply::target_mutation_lease& lease,
    const canonical_store& store);

} // namespace pkgstate::apply_adapter
