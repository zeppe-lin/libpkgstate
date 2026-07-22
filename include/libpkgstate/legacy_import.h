// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file legacy_import.h
 * \brief Explicit receipt-bound import of incomplete legacy state.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <libpkgstate/digest.h>
#include <libpkgstate/installed_control.h>
#include <libpkgstate/legacy_installed_package.h>
#include <libpkgstate/legacy_snapshot.h>
#include <libpkgstate/package_release.h>
#include <libpkgstate/publication_receipt.h>
#include <libpkgstate/snapshot.h>
#include <libpkgstate/state_target_binding.h>

namespace pkgstate {
namespace detail {

struct legacy_migration_evidence_reference_domain final {};

} // namespace detail

/*! \brief Caller-authoritative identity of explicit migration evidence. */
using legacy_migration_evidence_identity =
    detail::referenced_digest<
        detail::legacy_migration_evidence_reference_domain>;

/*! \brief Current explicit legacy import request schema. */
inline constexpr std::uint16_t legacy_import_request_schema_version = 1;

/*! \brief Current explicit legacy import receipt schema. */
inline constexpr std::uint16_t legacy_import_receipt_schema_version = 1;

/*!
 * \brief Durable identity projection of one historical source store.
 *
 * The source binding identifies where the incomplete observation came from.
 * It intentionally omits a publication-domain identity because reading the
 * source is not canonical state publication.
 */
class legacy_import_source final {
public:
  [[nodiscard]] static legacy_import_source
  make(managed_target_identity managed_target,
       state_store_identity state_store,
       root_view_identity root_view,
       state_backend_identity state_backend);

  [[nodiscard]] const managed_target_identity&
  managed_target() const noexcept;
  [[nodiscard]] const state_store_identity& state_store() const noexcept;
  [[nodiscard]] const root_view_identity& root_view() const noexcept;
  [[nodiscard]] const state_backend_identity& state_backend() const noexcept;

  friend bool operator==(const legacy_import_source& lhs,
                         const legacy_import_source& rhs) noexcept;
  friend bool operator!=(const legacy_import_source& lhs,
                         const legacy_import_source& rhs) noexcept;
  friend bool operator<(const legacy_import_source& lhs,
                        const legacy_import_source& rhs) noexcept;

private:
  legacy_import_source(managed_target_identity managed_target,
                       state_store_identity state_store,
                       root_view_identity root_view,
                       state_backend_identity state_backend);

  managed_target_identity managed_target_;
  state_store_identity state_store_;
  root_view_identity root_view_;
  state_backend_identity state_backend_;
};

/*!
 * \brief Explicit supply or historical absence of one external provenance fact.
 *
 * Every legacy package import must provide exactly one decision for each
 * candidate, artifact, application, and transaction provenance class.
 */
class legacy_import_provenance final {
public:
  /*! \brief Supply one provenance identity through explicit migration input. */
  [[nodiscard]] static legacy_import_provenance
  supplied(control_provenance_kind kind, std::string_view identity);

  /*! \brief Declare one provenance class historically unavailable. */
  [[nodiscard]] static legacy_import_provenance
  historically_unavailable(control_provenance_kind kind);

  [[nodiscard]] control_provenance_kind kind() const noexcept;
  [[nodiscard]] installed_control_fact_state state() const noexcept;
  [[nodiscard]] const std::optional<std::string>& identity() const noexcept;

  friend bool operator==(const legacy_import_provenance& lhs,
                         const legacy_import_provenance& rhs) noexcept;
  friend bool operator!=(const legacy_import_provenance& lhs,
                         const legacy_import_provenance& rhs) noexcept;
  friend bool operator<(const legacy_import_provenance& lhs,
                        const legacy_import_provenance& rhs) noexcept;

private:
  legacy_import_provenance(control_provenance_kind kind,
                           installed_control_fact_state state,
                           std::optional<std::string> identity);

  control_provenance_kind kind_;
  installed_control_fact_state state_;
  std::optional<std::string> identity_;
};

/*!
 * \brief Explicit canonical facts and absence decisions for one legacy package.
 *
 * The opaque legacy version line is never parsed. The caller supplies one
 * authoritative package_release and explicit control facts or explicit
 * historical absence. The source observation identity keeps the old fields
 * bound to the migration request.
 */
class legacy_package_import final {
public:
  [[nodiscard]] static legacy_package_import
  make(const legacy_installed_package& source,
       package_release release,
       installed_control_fact_state runtime_dependencies_state,
       std::vector<runtime_dependency_declaration> runtime_dependencies,
       installed_control_fact_state removal_lifecycle_state,
       std::vector<removal_lifecycle_declaration> removal_lifecycle,
       installed_control_fact_state target_profile_state,
       std::vector<target_profile_fact> target_profile,
       std::vector<legacy_import_provenance> provenance);

  [[nodiscard]] const std::string& package_name() const noexcept;
  [[nodiscard]] const legacy_package_observation_identity&
  source_package() const noexcept;
  [[nodiscard]] const package_release& release() const noexcept;
  [[nodiscard]] installed_control_fact_state
  runtime_dependencies_state() const noexcept;
  [[nodiscard]] const std::vector<runtime_dependency_declaration>&
  runtime_dependencies() const noexcept;
  [[nodiscard]] installed_control_fact_state
  removal_lifecycle_state() const noexcept;
  [[nodiscard]] const std::vector<removal_lifecycle_declaration>&
  removal_lifecycle() const noexcept;
  [[nodiscard]] installed_control_fact_state
  target_profile_state() const noexcept;
  [[nodiscard]] const std::vector<target_profile_fact>&
  target_profile() const noexcept;
  [[nodiscard]] const std::vector<legacy_import_provenance>&
  provenance() const noexcept;

  friend bool operator==(const legacy_package_import& lhs,
                         const legacy_package_import& rhs) noexcept;
  friend bool operator!=(const legacy_package_import& lhs,
                         const legacy_package_import& rhs) noexcept;
  friend bool operator<(const legacy_package_import& lhs,
                        const legacy_package_import& rhs) noexcept;

private:
  legacy_package_import(
      std::string package_name,
      legacy_package_observation_identity source_package,
      package_release release,
      installed_control_fact_state runtime_dependencies_state,
      std::vector<runtime_dependency_declaration> runtime_dependencies,
      installed_control_fact_state removal_lifecycle_state,
      std::vector<removal_lifecycle_declaration> removal_lifecycle,
      installed_control_fact_state target_profile_state,
      std::vector<target_profile_fact> target_profile,
      std::vector<legacy_import_provenance> provenance);

  std::string package_name_;
  legacy_package_observation_identity source_package_;
  package_release release_;
  installed_control_fact_state runtime_dependencies_state_;
  std::vector<runtime_dependency_declaration> runtime_dependencies_;
  installed_control_fact_state removal_lifecycle_state_;
  std::vector<removal_lifecycle_declaration> removal_lifecycle_;
  installed_control_fact_state target_profile_state_;
  std::vector<target_profile_fact> target_profile_;
  std::vector<legacy_import_provenance> provenance_;
};

/*!
 * \brief Immutable validated import from one exact legacy observation.
 *
 * Construction requires a fresh empty canonical destination, one explicit
 * package import for every source record, and one migration-evidence identity.
 * It preserves source manifests exactly, computes complete canonical installed
 * records, and exposes the resulting snapshot for non-mutating validation.
 */
class legacy_import_request final {
public:
  [[nodiscard]] static legacy_import_request
  make(legacy_import_source source,
       const legacy_snapshot& source_snapshot,
       const snapshot& expected_destination,
       std::vector<legacy_package_import> packages,
       legacy_migration_evidence_identity migration_evidence);

  [[nodiscard]] std::uint16_t schema_version() const noexcept;
  [[nodiscard]] const legacy_import_request_identity& identity() const noexcept;
  [[nodiscard]] const legacy_import_source& source() const noexcept;
  [[nodiscard]] const legacy_snapshot_observation_identity&
  source_snapshot() const noexcept;
  [[nodiscard]] const installed_state_snapshot_identity&
  expected_destination() const noexcept;
  [[nodiscard]] const state_target_binding&
  destination_binding() const noexcept;
  [[nodiscard]] const legacy_migration_evidence_identity&
  migration_evidence() const noexcept;
  [[nodiscard]] const std::vector<legacy_package_import>&
  packages() const noexcept;
  [[nodiscard]] const snapshot& resulting_snapshot() const noexcept;

private:
  legacy_import_request(
      legacy_import_request_identity identity,
      legacy_import_source source,
      legacy_snapshot_observation_identity source_snapshot,
      installed_state_snapshot_identity expected_destination,
      state_target_binding destination_binding,
      legacy_migration_evidence_identity migration_evidence,
      std::vector<legacy_package_import> packages,
      snapshot resulting_snapshot);

  legacy_import_request_identity identity_;
  legacy_import_source source_;
  legacy_snapshot_observation_identity source_snapshot_;
  installed_state_snapshot_identity expected_destination_;
  state_target_binding destination_binding_;
  legacy_migration_evidence_identity migration_evidence_;
  std::vector<legacy_package_import> packages_;
  snapshot resulting_snapshot_;
};

/*! \brief Typed outcome of one explicit legacy import operation. */
enum class legacy_import_outcome : std::uint8_t {
  validated_only = 1,
  imported = 2,
  stale_destination = 3,
  request_rejected = 4,
  failed_before_publication = 5,
  imported_durability_unconfirmed = 6,
  indeterminate = 7,
};

/*!
 * \brief Immutable evidence of validation or attempted canonical import.
 */
class legacy_import_receipt final {
public:
  [[nodiscard]] static legacy_import_receipt
  validated(const legacy_import_request& request);

  [[nodiscard]] static legacy_import_receipt
  imported(const legacy_import_request& request,
           const snapshot& actual_prior,
           std::string storage_format,
           state_storage_atomicity_boundary atomicity,
           std::vector<state_publication_evidence_identity> evidence = {});

  [[nodiscard]] static legacy_import_receipt
  stale_destination(
      const legacy_import_request& request,
      const snapshot& actual_prior,
      std::string storage_format,
      std::vector<state_publication_evidence_identity> evidence = {});

  [[nodiscard]] static legacy_import_receipt
  request_rejected(
      const legacy_import_request& request,
      const snapshot& actual_prior,
      std::string storage_format,
      std::vector<state_publication_evidence_identity> evidence = {});

  [[nodiscard]] static legacy_import_receipt
  failed_before_publication(
      const legacy_import_request& request,
      const snapshot& actual_prior,
      std::string storage_format,
      std::vector<state_publication_evidence_identity> evidence = {});

  [[nodiscard]] static legacy_import_receipt
  imported_but_durability_unconfirmed(
      const legacy_import_request& request,
      const snapshot& actual_prior,
      std::string storage_format,
      state_storage_atomicity_boundary atomicity,
      std::vector<state_publication_evidence_identity> evidence = {});

  [[nodiscard]] static legacy_import_receipt
  indeterminate(
      const legacy_import_request& request,
      const snapshot& actual_prior,
      bool resulting_snapshot_established,
      std::string storage_format,
      state_storage_atomicity_boundary atomicity,
      std::vector<state_publication_evidence_identity> evidence = {});

  [[nodiscard]] std::uint16_t schema_version() const noexcept;
  [[nodiscard]] const legacy_import_receipt_identity& identity() const noexcept;
  [[nodiscard]] const legacy_import_request_identity& request() const noexcept;
  [[nodiscard]] const legacy_snapshot_observation_identity&
  source_snapshot() const noexcept;
  [[nodiscard]] const installed_state_snapshot_identity&
  expected_destination() const noexcept;
  [[nodiscard]] const installed_state_snapshot_identity&
  actual_destination() const noexcept;
  [[nodiscard]] const state_target_binding&
  destination_binding() const noexcept;
  [[nodiscard]] legacy_import_outcome outcome() const noexcept;
  [[nodiscard]] state_publication_durability durability() const noexcept;
  [[nodiscard]] state_storage_atomicity_boundary
  atomicity_boundary() const noexcept;
  [[nodiscard]] const std::optional<installed_state_snapshot_identity>&
  resulting_snapshot() const noexcept;
  [[nodiscard]] const std::optional<std::string>&
  storage_format() const noexcept;
  [[nodiscard]] const std::vector<state_publication_evidence_identity>&
  subordinate_evidence() const noexcept;

private:
  [[nodiscard]] static legacy_import_receipt
  make(const legacy_import_request& request,
       const snapshot& actual_prior,
       legacy_import_outcome outcome,
       state_publication_durability durability,
       state_storage_atomicity_boundary atomicity,
       std::optional<installed_state_snapshot_identity> resulting_snapshot,
       std::optional<std::string> storage_format,
       std::vector<state_publication_evidence_identity> evidence);

  legacy_import_receipt(
      legacy_import_receipt_identity identity,
      legacy_import_request_identity request,
      legacy_snapshot_observation_identity source_snapshot,
      installed_state_snapshot_identity expected_destination,
      installed_state_snapshot_identity actual_destination,
      state_target_binding destination_binding,
      legacy_import_outcome outcome,
      state_publication_durability durability,
      state_storage_atomicity_boundary atomicity,
      std::optional<installed_state_snapshot_identity> resulting_snapshot,
      std::optional<std::string> storage_format,
      std::vector<state_publication_evidence_identity> evidence);

  legacy_import_receipt_identity identity_;
  legacy_import_request_identity request_;
  legacy_snapshot_observation_identity source_snapshot_;
  installed_state_snapshot_identity expected_destination_;
  installed_state_snapshot_identity actual_destination_;
  state_target_binding destination_binding_;
  legacy_import_outcome outcome_;
  state_publication_durability durability_;
  state_storage_atomicity_boundary atomicity_;
  std::optional<installed_state_snapshot_identity> resulting_snapshot_;
  std::optional<std::string> storage_format_;
  std::vector<state_publication_evidence_identity> evidence_;
};

} // namespace pkgstate
