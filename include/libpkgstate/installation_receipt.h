// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file installation_receipt.h
 *  \brief Complete native evidence admitted for installed-state publication.
 */
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <libpkgstate/digest.h>
#include <libpkgstate/installed_control.h>
#include <libpkgstate/owned_entry.h>
#include <libpkgstate/state_target_binding.h>

namespace pkgstate {

inline constexpr std::uint16_t installation_receipt_schema_version = 1;

class installation_receipt final {
public:
  [[nodiscard]] static installation_receipt make(
      installed_control control,
      state_target_binding target_binding,
      std::vector<owned_entry> manifest,
      operation_plan_identity operation_plan,
      application_evidence_identity application_evidence,
      std::optional<transaction_evidence_identity> transaction_evidence = std::nullopt);

  [[nodiscard]] std::uint16_t schema_version() const noexcept;
  [[nodiscard]] const installation_receipt_identity& identity() const noexcept;
  [[nodiscard]] const installed_control& control() const noexcept;
  [[nodiscard]] const package_release& release() const noexcept;
  [[nodiscard]] const state_target_binding& target_binding() const noexcept;
  [[nodiscard]] const std::vector<owned_entry>& manifest() const noexcept;
  [[nodiscard]] const operation_plan_identity& operation_plan() const noexcept;
  [[nodiscard]] const application_evidence_identity& application_evidence() const noexcept;
  [[nodiscard]] const std::optional<transaction_evidence_identity>& transaction_evidence() const noexcept;

  friend bool operator==(const installation_receipt& lhs,
                         const installation_receipt& rhs) noexcept;
  friend bool operator!=(const installation_receipt& lhs,
                         const installation_receipt& rhs) noexcept;
  friend bool operator<(const installation_receipt& lhs,
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
