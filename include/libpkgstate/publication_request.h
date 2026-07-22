// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file publication_request.h
 * \brief Immutable stale-safe installed-state publication requests.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <libpkgstate/digest.h>
#include <libpkgstate/installed_package.h>
#include <libpkgstate/snapshot.h>
#include <libpkgstate/state_target_binding.h>

namespace pkgstate {
namespace detail {

struct operation_plan_reference_domain final {};
struct application_evidence_reference_domain final {};
struct transaction_evidence_reference_domain final {};

} // namespace detail

/*! \brief Caller-authoritative identity of one accepted operation plan. */
using operation_plan_identity =
    detail::referenced_digest<detail::operation_plan_reference_domain>;

/*! \brief Caller-authoritative identity of completed application evidence. */
using application_evidence_identity =
    detail::referenced_digest<detail::application_evidence_reference_domain>;

/*! \brief Caller-authoritative identity of one completed transaction. */
using transaction_evidence_identity =
    detail::referenced_digest<detail::transaction_evidence_reference_domain>;

/*! \brief Current canonical state-publication request schema. */
inline constexpr std::uint16_t state_publication_request_schema_version = 1;

/*! \brief Semantic installed-state transition of one package name. */
enum class package_state_delta_kind : std::uint8_t {
  install = 1, //!< Require absence and add one complete installed package.
  replace = 2, //!< Require one exact installed package and replace it.
  remove = 3,  //!< Require one exact installed package and make it absent.
};

/*!
 * \brief One immutable package-state consequence eligible for publication.
 *
 * Install and replacement deltas contain the complete proposed canonical
 * installed package.  Replacement and removal deltas contain the exact prior
 * installed-package identity expected beneath the request snapshot.  Every
 * delta carries one accepted operation-plan identity and one completed
 * application-evidence identity.
 */
class package_state_delta final {
public:
  /*! \brief Construct an installation delta. */
  [[nodiscard]] static package_state_delta
  install(installed_package proposed,
          operation_plan_identity operation_plan,
          application_evidence_identity application_evidence);

  /*! \brief Construct an exact replacement delta. */
  [[nodiscard]] static package_state_delta
  replace(installed_package_identity expected_package,
          installed_package proposed,
          operation_plan_identity operation_plan,
          application_evidence_identity application_evidence);

  /*! \brief Construct an exact removal delta. */
  [[nodiscard]] static package_state_delta
  remove(std::string_view package_name,
         installed_package_identity expected_package,
         operation_plan_identity operation_plan,
         application_evidence_identity application_evidence);

  /*! \brief Return the transition class. */
  [[nodiscard]] package_state_delta_kind kind() const noexcept;

  /*! \brief Return the affected canonical package name. */
  [[nodiscard]] const std::string& package_name() const noexcept;

  /*! \brief Return the required old installed-package identity, when any. */
  [[nodiscard]] const std::optional<installed_package_identity>&
  expected_package() const noexcept;

  /*! \brief Return the proposed complete installed package, when any. */
  [[nodiscard]] const std::optional<installed_package>&
  proposed_package() const noexcept;

  /*! \brief Return the accepted operation-plan identity. */
  [[nodiscard]] const operation_plan_identity&
  operation_plan() const noexcept;

  /*! \brief Return the completed application-evidence identity. */
  [[nodiscard]] const application_evidence_identity&
  application_evidence() const noexcept;

  friend bool operator==(const package_state_delta& lhs,
                         const package_state_delta& rhs) noexcept;
  friend bool operator!=(const package_state_delta& lhs,
                         const package_state_delta& rhs) noexcept;
  friend bool operator<(const package_state_delta& lhs,
                        const package_state_delta& rhs) noexcept;

private:
  package_state_delta(package_state_delta_kind kind,
                      std::string package_name,
                      std::optional<installed_package_identity>
                          expected_package,
                      std::optional<installed_package> proposed_package,
                      operation_plan_identity operation_plan,
                      application_evidence_identity application_evidence);

  package_state_delta_kind kind_;
  std::string package_name_;
  std::optional<installed_package_identity> expected_package_;
  std::optional<installed_package> proposed_package_;
  operation_plan_identity operation_plan_;
  application_evidence_identity application_evidence_;
};

/*!
 * \brief Immutable request to compare and publish installed-state consequences.
 *
 * Construction takes one complete expected snapshot rather than independent
 * snapshot and target identities.  It validates every delta against that
 * snapshot, rejects conflicting package names, requires transaction evidence
 * for a composed request, and computes one request identity.  It does not
 * mutate a store and is not an arbitrary caller-authored replacement snapshot.
 */
class state_publication_request final {
public:
  /*! \brief Validate, normalize, identify, and construct a request. */
  [[nodiscard]] static state_publication_request
  make(const snapshot& expected_snapshot,
       std::vector<package_state_delta> deltas,
       std::optional<transaction_evidence_identity>
           transaction_evidence = std::nullopt);

  /*! \brief Return the canonical request schema version. */
  [[nodiscard]] std::uint16_t schema_version() const noexcept;

  /*! \brief Return the state-computed publication-request identity. */
  [[nodiscard]] const state_publication_request_identity&
  identity() const noexcept;

  /*! \brief Return the exact prior snapshot required by the request. */
  [[nodiscard]] const installed_state_snapshot_identity&
  expected_snapshot() const noexcept;

  /*! \brief Return the target-state binding of the expected snapshot. */
  [[nodiscard]] const state_target_binding&
  target_binding() const noexcept;

  /*! \brief Return package deltas in canonical package-name order. */
  [[nodiscard]] const std::vector<package_state_delta>&
  deltas() const noexcept;

  /*! \brief Return transaction evidence, when the request carries it. */
  [[nodiscard]] const std::optional<transaction_evidence_identity>&
  transaction_evidence() const noexcept;

private:
  state_publication_request(
      state_publication_request_identity identity,
      installed_state_snapshot_identity expected_snapshot,
      state_target_binding target_binding,
      std::vector<package_state_delta> deltas,
      std::optional<transaction_evidence_identity> transaction_evidence);

  state_publication_request_identity identity_;
  installed_state_snapshot_identity expected_snapshot_;
  state_target_binding target_binding_;
  std::vector<package_state_delta> deltas_;
  std::optional<transaction_evidence_identity> transaction_evidence_;
};

} // namespace pkgstate
