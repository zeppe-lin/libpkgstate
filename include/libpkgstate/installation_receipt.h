// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file installation_receipt.h
 * \brief Complete admitted evidence for one installed package publication.
 */
#pragma once

#include <libpkgstate/export.h>

#include <cstdint>
#include <optional>
#include <vector>

#include <libpkgstate/digest.h>
#include <libpkgstate/installed_control.h>
#include <libpkgstate/owned_entry.h>
#include <libpkgstate/state_target_binding.h>

namespace pkgstate {

/*! \brief Canonical semantic schema of installation_receipt identity input. */
inline constexpr std::uint16_t installation_receipt_schema_version = 2;

/*!
 * \brief Immutable package installation authority admitted into state.
 *
 * A receipt binds complete source and build control, the exact managed target,
 * the normalized installed object manifest, the operation plan, completed
 * application evidence, and optional transaction evidence. It is not an
 * execution log and cannot be assembled from partial filesystem observation.
 */
class PKGSTATE_API installation_receipt final {
public:
  /*!
   * \brief Validate, normalize, and identify one installation receipt.
   * \param control Complete installed source and build control.
   * \param target_binding Exact durable target-state authority.
   * \param manifest Non-empty installed object manifest, normalized by class.
   * \param operation_plan Planner-owned identity of the exact applied plan.
   * \param application_evidence Application-owned completion identity.
   * \param transaction_evidence Optional orchestration-owned transaction proof.
   * \return Immutable identified installation receipt.
   * \throws state_error for duplicate paths, invalid hard-link topology,
   * mismatched package authority, or otherwise inconsistent evidence.
   */
  [[nodiscard]] static installation_receipt make(
      installed_control control,
      state_target_binding target_binding,
      std::vector<owned_entry> manifest,
      operation_plan_identity operation_plan,
      application_evidence_identity application_evidence,
      std::optional<transaction_evidence_identity> transaction_evidence =
          std::nullopt);

  /*!
   * \brief Return installation_receipt_schema_version.
   * \return Installation_receipt_schema_version.
   */
  [[nodiscard]] std::uint16_t schema_version() const noexcept;
  /*!
   * \brief Return the canonical receipt identity.
   * \return The canonical receipt identity.
   */
  [[nodiscard]] const installation_receipt_identity& identity() const noexcept;
  /*!
   * \brief Return complete installed source and build control.
   * \return Complete installed source and build control.
   */
  [[nodiscard]] const installed_control& control() const noexcept;
  /*!
   * \brief Return the source-authoritative package release.
   * \return The source-authoritative package release.
   */
  [[nodiscard]] const package_release& release() const noexcept;
  /*!
   * \brief Return the exact durable target-state binding.
   * \return The exact durable target-state binding.
   */
  [[nodiscard]] const state_target_binding& target_binding() const noexcept;
  /*!
   * \brief Return the canonical installed object manifest.
   * \return The canonical installed object manifest.
   */
  [[nodiscard]] const std::vector<owned_entry>& manifest() const noexcept;
  /*!
   * \brief Return the exact planner-owned operation-plan identity.
   * \return The exact planner-owned operation-plan identity.
   */
  [[nodiscard]] const operation_plan_identity& operation_plan() const noexcept;
  /*!
   * \brief Return the exact completed-application evidence identity.
   * \return The exact completed-application evidence identity.
   */
  [[nodiscard]] const application_evidence_identity&
  application_evidence() const noexcept;
  /*!
   * \brief Return optional orchestration-owned transaction evidence.
   * \return Optional orchestration-owned transaction evidence.
   */
  [[nodiscard]] const std::optional<transaction_evidence_identity>&
  transaction_evidence() const noexcept;

  /*!
   * \brief Compare complete installation receipts for equality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands are equal.
   */
  friend PKGSTATE_API bool operator==(const installation_receipt& lhs,
                                      const installation_receipt& rhs) noexcept;
  /*!
   * \brief Compare complete installation receipts for inequality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands differ.
   */
  friend PKGSTATE_API bool operator!=(const installation_receipt& lhs,
                                      const installation_receipt& rhs) noexcept;
  /*!
   * \brief Order installation receipts canonically.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the left operand precedes the right operand.
   */
  friend PKGSTATE_API bool operator<(const installation_receipt& lhs,
                                     const installation_receipt& rhs) noexcept;

private:
  installation_receipt(
      installation_receipt_identity identity,
      installed_control control,
      state_target_binding target_binding,
      std::vector<owned_entry> manifest,
      operation_plan_identity operation_plan,
      application_evidence_identity application_evidence,
      std::optional<transaction_evidence_identity> transaction_evidence);

  installation_receipt_identity identity_;
  installed_control control_;
  state_target_binding target_binding_;
  std::vector<owned_entry> manifest_;
  operation_plan_identity operation_plan_;
  application_evidence_identity application_evidence_;
  std::optional<transaction_evidence_identity> transaction_evidence_;
};

} // namespace pkgstate
