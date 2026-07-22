// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file publication_receipt.h
 * \brief Typed immutable installed-state publication outcomes.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <libpkgstate/digest.h>
#include <libpkgstate/publication_request.h>
#include <libpkgstate/snapshot.h>
#include <libpkgstate/state_target_binding.h>

namespace pkgstate {
namespace detail {

struct state_publication_evidence_reference_domain final {};

} // namespace detail

/*! \brief Identity of backend-authoritative publication evidence. */
using state_publication_evidence_identity =
    detail::referenced_digest<
        detail::state_publication_evidence_reference_domain>;

/*! \brief Current canonical state-publication receipt schema. */
inline constexpr std::uint16_t state_publication_receipt_schema_version = 1;

/*! \brief Semantic outcome of one installed-state publication attempt. */
enum class state_publication_outcome : std::uint8_t {
  published = 1, //!< Resulting state was published and durability confirmed.
  stale_expected_state = 2, //!< Actual prior state differed; no mutation.
  request_rejected = 3, //!< Request was rejected before publication.
  failed_before_publication = 4, //!< Attempt failed before state publication.
  published_durability_unconfirmed = 5, //!< Visible, durability unconfirmed.
  indeterminate = 6, //!< Publication visibility requires an authoritative read.
};

/*! \brief Durability knowledge established by one publication attempt. */
enum class state_publication_durability : std::uint8_t {
  not_attempted = 1, //!< No state publication was attempted.
  confirmed = 2, //!< Publication durability was confirmed.
  unconfirmed = 3, //!< State was published but durability was not confirmed.
  indeterminate = 4, //!< Visibility or durability requires rereading state.
};

/*! \brief Actual atomic state-storage boundary used by the backend. */
enum class state_storage_atomicity_boundary : std::uint8_t {
  none = 1, //!< No state-storage publication boundary was crossed.
  complete_state_object_replace = 2, //!< Complete object replacement.
  immutable_generation_selection = 3, //!< Generation selection.
};

/*!
 * \brief Immutable typed evidence of one state-publication attempt.
 *
 * Factories enforce coherent outcome, durability, and atomicity combinations.
 * They bind one request to the actual prior snapshot observed under the
 * publication boundary.  Successful outcomes additionally validate the exact
 * resulting snapshot implied by the request deltas.  Receipt construction does
 * not mutate storage and does not claim filesystem/state global atomicity.
 */
class state_publication_receipt final {
public:
  /*! \brief Record confirmed successful state publication. */
  [[nodiscard]] static state_publication_receipt
  published(const state_publication_request& request,
            const snapshot& actual_prior,
            const snapshot& resulting_snapshot,
            std::string storage_format,
            state_storage_atomicity_boundary atomicity,
            std::vector<state_publication_evidence_identity>
                subordinate_evidence = {});

  /*! \brief Record an ordinary stale-expected-state refusal. */
  [[nodiscard]] static state_publication_receipt
  stale_expected_state(
      const state_publication_request& request,
      const snapshot& actual_prior,
      std::string storage_format,
      std::vector<state_publication_evidence_identity>
          subordinate_evidence = {});

  /*! \brief Record semantic request rejection before publication. */
  [[nodiscard]] static state_publication_receipt
  request_rejected(
      const state_publication_request& request,
      const snapshot& actual_prior,
      std::string storage_format,
      std::vector<state_publication_evidence_identity>
          subordinate_evidence = {});

  /*! \brief Record failure after comparison but before publication. */
  [[nodiscard]] static state_publication_receipt
  failed_before_publication(
      const state_publication_request& request,
      const snapshot& actual_prior,
      std::string storage_format,
      std::vector<state_publication_evidence_identity>
          subordinate_evidence = {});

  /*! \brief Record publication whose durability confirmation failed. */
  [[nodiscard]] static state_publication_receipt
  published_but_durability_unconfirmed(
      const state_publication_request& request,
      const snapshot& actual_prior,
      const snapshot& resulting_snapshot,
      std::string storage_format,
      state_storage_atomicity_boundary atomicity,
      std::vector<state_publication_evidence_identity>
          subordinate_evidence = {});

  /*! \brief Record an attempt whose authoritative state requires rereading. */
  [[nodiscard]] static state_publication_receipt
  indeterminate(
      const state_publication_request& request,
      const snapshot& actual_prior,
      std::optional<installed_state_snapshot_identity> resulting_snapshot,
      std::string storage_format,
      state_storage_atomicity_boundary atomicity,
      std::vector<state_publication_evidence_identity>
          subordinate_evidence = {});

  /*! \brief Return the canonical receipt schema version. */
  [[nodiscard]] std::uint16_t schema_version() const noexcept;

  /*! \brief Return the state-computed publication-receipt identity. */
  [[nodiscard]] const state_publication_receipt_identity&
  identity() const noexcept;

  /*! \brief Return the immutable request represented by this outcome. */
  [[nodiscard]] const state_publication_request_identity&
  request() const noexcept;

  /*! \brief Return the prior snapshot required by the request. */
  [[nodiscard]] const installed_state_snapshot_identity&
  expected_prior_snapshot() const noexcept;

  /*! \brief Return the actual prior snapshot observed by the backend. */
  [[nodiscard]] const installed_state_snapshot_identity&
  actual_prior_snapshot() const noexcept;

  /*! \brief Return the durable target and store binding. */
  [[nodiscard]] const state_target_binding&
  target_binding() const noexcept;

  /*! \brief Return the durable state-store identity. */
  [[nodiscard]] const state_store_identity& state_store() const noexcept;

  /*! \brief Return the state-backend identity. */
  [[nodiscard]] const state_backend_identity& backend() const noexcept;

  /*! \brief Return the backend storage-format identifier. */
  [[nodiscard]] const std::string& storage_format() const noexcept;

  /*! \brief Return the typed publication outcome. */
  [[nodiscard]] state_publication_outcome outcome() const noexcept;

  /*! \brief Return the established durability knowledge. */
  [[nodiscard]] state_publication_durability durability() const noexcept;

  /*! \brief Return the actual state-storage atomicity boundary. */
  [[nodiscard]] state_storage_atomicity_boundary
  atomicity_boundary() const noexcept;

  /*! \brief Return the resulting snapshot identity, when established. */
  [[nodiscard]] const std::optional<installed_state_snapshot_identity>&
  resulting_snapshot() const noexcept;

  /*! \brief Return subordinate evidence in canonical identity order. */
  [[nodiscard]] const std::vector<state_publication_evidence_identity>&
  subordinate_evidence() const noexcept;

  friend bool operator==(const state_publication_receipt& lhs,
                         const state_publication_receipt& rhs) noexcept;
  friend bool operator!=(const state_publication_receipt& lhs,
                         const state_publication_receipt& rhs) noexcept;
  friend bool operator<(const state_publication_receipt& lhs,
                        const state_publication_receipt& rhs) noexcept;

private:
  [[nodiscard]] static state_publication_receipt
  make(const state_publication_request& request,
       const snapshot& actual_prior,
       std::string storage_format,
       state_publication_outcome outcome,
       state_publication_durability durability,
       state_storage_atomicity_boundary atomicity,
       std::optional<installed_state_snapshot_identity> resulting_snapshot,
       std::vector<state_publication_evidence_identity>
           subordinate_evidence);

  state_publication_receipt(
      state_publication_receipt_identity identity,
      state_publication_request_identity request,
      installed_state_snapshot_identity expected_prior_snapshot,
      installed_state_snapshot_identity actual_prior_snapshot,
      state_target_binding target_binding,
      std::string storage_format,
      state_publication_outcome outcome,
      state_publication_durability durability,
      state_storage_atomicity_boundary atomicity_boundary,
      std::optional<installed_state_snapshot_identity> resulting_snapshot,
      std::vector<state_publication_evidence_identity> subordinate_evidence);

  state_publication_receipt_identity identity_;
  state_publication_request_identity request_;
  installed_state_snapshot_identity expected_prior_snapshot_;
  installed_state_snapshot_identity actual_prior_snapshot_;
  state_target_binding target_binding_;
  std::string storage_format_;
  state_publication_outcome outcome_;
  state_publication_durability durability_;
  state_storage_atomicity_boundary atomicity_boundary_;
  std::optional<installed_state_snapshot_identity> resulting_snapshot_;
  std::vector<state_publication_evidence_identity> subordinate_evidence_;
};

} // namespace pkgstate
