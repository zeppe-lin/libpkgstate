// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/publication_receipt.h>

#include "canonical_record.h"
#include "publication_projection.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <libpkgstate/error.h>

namespace pkgstate {
namespace {

void
validate_storage_format(std::string_view storage_format)
{
  if (storage_format.empty())
    throw state_error("publication receipt storage format must not be empty");

  for (const char byte : storage_format)
  {
    if (byte == '\0' || byte == '\n' || byte == '\r')
    {
      throw state_error(
          "publication receipt storage format must be line-safe");
    }
  }
}

void
validate_target(const state_publication_request& request,
                const snapshot& actual_prior)
{
  if (actual_prior.target_binding() != request.target_binding())
  {
    throw state_error(
        "publication receipt actual prior belongs to another target");
  }
}

void
require_current(const state_publication_request& request,
                const snapshot& actual_prior)
{
  validate_target(request, actual_prior);
  if (actual_prior.identity() != request.expected_snapshot())
  {
    throw state_error(
        "publication receipt requires the expected prior snapshot");
  }
}

void
require_stale(const state_publication_request& request,
              const snapshot& actual_prior)
{
  validate_target(request, actual_prior);
  if (actual_prior.identity() == request.expected_snapshot())
  {
    throw state_error(
        "stale-state receipt requires a different actual prior snapshot");
  }
}

void
require_publication_boundary(state_storage_atomicity_boundary atomicity)
{
  if (atomicity == state_storage_atomicity_boundary::none)
  {
    throw state_error(
        "publication receipt requires a state-storage atomicity boundary");
  }
}

std::vector<state_publication_evidence_identity>
normalize_evidence(
    std::vector<state_publication_evidence_identity> subordinate_evidence)
{
  std::sort(subordinate_evidence.begin(), subordinate_evidence.end());
  if (std::adjacent_find(subordinate_evidence.begin(),
                         subordinate_evidence.end()) !=
      subordinate_evidence.end())
  {
    throw state_error(
        "publication receipt contains duplicate subordinate evidence");
  }
  return subordinate_evidence;
}

installed_state_snapshot_identity
validate_result(const state_publication_request& request,
                const snapshot& actual_prior,
                const snapshot& resulting_snapshot)
{
  if (resulting_snapshot.target_binding() != request.target_binding())
  {
    throw state_error(
        "publication receipt result belongs to another target");
  }

  const snapshot expected_result =
      detail::project_publication_request(request, actual_prior);
  if (resulting_snapshot.identity() != expected_result.identity())
  {
    throw state_error(
        "publication receipt result does not match request deltas");
  }

  return resulting_snapshot.identity();
}

std::uint8_t
canonical_outcome(state_publication_outcome outcome)
{
  switch (outcome)
  {
    case state_publication_outcome::published:
      return 1;
    case state_publication_outcome::stale_expected_state:
      return 2;
    case state_publication_outcome::request_rejected:
      return 3;
    case state_publication_outcome::failed_before_publication:
      return 4;
    case state_publication_outcome::published_durability_unconfirmed:
      return 5;
    case state_publication_outcome::indeterminate:
      return 6;
  }
  throw state_error("invalid state-publication outcome");
}

std::uint8_t
canonical_durability(state_publication_durability durability)
{
  switch (durability)
  {
    case state_publication_durability::not_attempted:
      return 1;
    case state_publication_durability::confirmed:
      return 2;
    case state_publication_durability::unconfirmed:
      return 3;
    case state_publication_durability::indeterminate:
      return 4;
  }
  throw state_error("invalid state-publication durability");
}

std::uint8_t
canonical_atomicity(state_storage_atomicity_boundary atomicity)
{
  switch (atomicity)
  {
    case state_storage_atomicity_boundary::none:
      return 1;
    case state_storage_atomicity_boundary::complete_state_object_replace:
      return 2;
    case state_storage_atomicity_boundary::immutable_generation_selection:
      return 3;
  }
  throw state_error("invalid state-storage atomicity boundary");
}

state_publication_receipt_identity
identify_receipt(
    const state_publication_request& request,
    const snapshot& actual_prior,
    const std::string& storage_format,
    state_publication_outcome outcome,
    state_publication_durability durability,
    state_storage_atomicity_boundary atomicity,
    const std::optional<installed_state_snapshot_identity>& resulting_snapshot,
    const std::vector<state_publication_evidence_identity>&
        subordinate_evidence)
{
  detail::canonical_record record(
      state_publication_receipt_identity::canonical_domain());
  record.append_u16(state_publication_receipt_schema_version);
  record.append_digest(request.identity());
  record.append_digest(request.expected_snapshot());
  record.append_digest(actual_prior.identity());
  record.append_digest(request.target_binding().identity());
  record.append_bytes(storage_format);
  record.append_u8(canonical_outcome(outcome));
  record.append_u8(canonical_durability(durability));
  record.append_u8(canonical_atomicity(atomicity));
  record.append_bool(resulting_snapshot.has_value());
  if (resulting_snapshot.has_value())
    record.append_digest(*resulting_snapshot);
  record.append_u64(
      static_cast<std::uint64_t>(subordinate_evidence.size()));
  for (const state_publication_evidence_identity& evidence :
       subordinate_evidence)
  {
    record.append_digest(evidence);
  }

  return state_publication_receipt_identity::from_sha256(record.sha256());
}

} // namespace

state_publication_receipt
state_publication_receipt::make(
    const state_publication_request& request,
    const snapshot& actual_prior,
    std::string storage_format,
    state_publication_outcome outcome,
    state_publication_durability durability,
    state_storage_atomicity_boundary atomicity,
    std::optional<installed_state_snapshot_identity> resulting_snapshot,
    std::vector<state_publication_evidence_identity> subordinate_evidence)
{
  validate_storage_format(storage_format);
  subordinate_evidence = normalize_evidence(std::move(subordinate_evidence));
  state_publication_receipt_identity identity = identify_receipt(
      request, actual_prior, storage_format, outcome, durability, atomicity,
      resulting_snapshot, subordinate_evidence);

  return state_publication_receipt(
      std::move(identity), request.identity(), request.expected_snapshot(),
      actual_prior.identity(), request.target_binding(),
      std::move(storage_format), outcome, durability, atomicity,
      std::move(resulting_snapshot), std::move(subordinate_evidence));
}

state_publication_receipt
state_publication_receipt::published(
    const state_publication_request& request,
    const snapshot& actual_prior,
    const snapshot& resulting_snapshot,
    std::string storage_format,
    state_storage_atomicity_boundary atomicity,
    std::vector<state_publication_evidence_identity> subordinate_evidence)
{
  require_current(request, actual_prior);
  require_publication_boundary(atomicity);
  installed_state_snapshot_identity resulting =
      validate_result(request, actual_prior, resulting_snapshot);
  return make(
      request, actual_prior, std::move(storage_format),
      state_publication_outcome::published,
      state_publication_durability::confirmed, atomicity,
      std::move(resulting), std::move(subordinate_evidence));
}

state_publication_receipt
state_publication_receipt::stale_expected_state(
    const state_publication_request& request,
    const snapshot& actual_prior,
    std::string storage_format,
    std::vector<state_publication_evidence_identity> subordinate_evidence)
{
  require_stale(request, actual_prior);
  return make(
      request, actual_prior, std::move(storage_format),
      state_publication_outcome::stale_expected_state,
      state_publication_durability::not_attempted,
      state_storage_atomicity_boundary::none, std::nullopt,
      std::move(subordinate_evidence));
}

state_publication_receipt
state_publication_receipt::request_rejected(
    const state_publication_request& request,
    const snapshot& actual_prior,
    std::string storage_format,
    std::vector<state_publication_evidence_identity> subordinate_evidence)
{
  require_current(request, actual_prior);
  return make(
      request, actual_prior, std::move(storage_format),
      state_publication_outcome::request_rejected,
      state_publication_durability::not_attempted,
      state_storage_atomicity_boundary::none, std::nullopt,
      std::move(subordinate_evidence));
}

state_publication_receipt
state_publication_receipt::failed_before_publication(
    const state_publication_request& request,
    const snapshot& actual_prior,
    std::string storage_format,
    std::vector<state_publication_evidence_identity> subordinate_evidence)
{
  require_current(request, actual_prior);
  return make(
      request, actual_prior, std::move(storage_format),
      state_publication_outcome::failed_before_publication,
      state_publication_durability::not_attempted,
      state_storage_atomicity_boundary::none, std::nullopt,
      std::move(subordinate_evidence));
}

state_publication_receipt
state_publication_receipt::published_but_durability_unconfirmed(
    const state_publication_request& request,
    const snapshot& actual_prior,
    const snapshot& resulting_snapshot,
    std::string storage_format,
    state_storage_atomicity_boundary atomicity,
    std::vector<state_publication_evidence_identity> subordinate_evidence)
{
  require_current(request, actual_prior);
  require_publication_boundary(atomicity);
  installed_state_snapshot_identity resulting =
      validate_result(request, actual_prior, resulting_snapshot);
  return make(
      request, actual_prior, std::move(storage_format),
      state_publication_outcome::published_durability_unconfirmed,
      state_publication_durability::unconfirmed, atomicity,
      std::move(resulting), std::move(subordinate_evidence));
}

state_publication_receipt
state_publication_receipt::indeterminate(
    const state_publication_request& request,
    const snapshot& actual_prior,
    std::optional<installed_state_snapshot_identity> resulting_snapshot,
    std::string storage_format,
    state_storage_atomicity_boundary atomicity,
    std::vector<state_publication_evidence_identity> subordinate_evidence)
{
  require_current(request, actual_prior);
  require_publication_boundary(atomicity);

  if (resulting_snapshot.has_value())
  {
    const snapshot expected_result =
      detail::project_publication_request(request, actual_prior);
    if (*resulting_snapshot != expected_result.identity())
    {
      throw state_error(
          "indeterminate receipt cites an impossible resulting snapshot");
    }
  }

  return make(
      request, actual_prior, std::move(storage_format),
      state_publication_outcome::indeterminate,
      state_publication_durability::indeterminate, atomicity,
      std::move(resulting_snapshot), std::move(subordinate_evidence));
}

state_publication_receipt::state_publication_receipt(
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
    std::vector<state_publication_evidence_identity> subordinate_evidence)
    : identity_(std::move(identity)),
      request_(std::move(request)),
      expected_prior_snapshot_(std::move(expected_prior_snapshot)),
      actual_prior_snapshot_(std::move(actual_prior_snapshot)),
      target_binding_(std::move(target_binding)),
      storage_format_(std::move(storage_format)),
      outcome_(outcome),
      durability_(durability),
      atomicity_boundary_(atomicity_boundary),
      resulting_snapshot_(std::move(resulting_snapshot)),
      subordinate_evidence_(std::move(subordinate_evidence))
{
}

std::uint16_t
state_publication_receipt::schema_version() const noexcept
{
  return state_publication_receipt_schema_version;
}

const state_publication_receipt_identity&
state_publication_receipt::identity() const noexcept
{
  return identity_;
}

const state_publication_request_identity&
state_publication_receipt::request() const noexcept
{
  return request_;
}

const installed_state_snapshot_identity&
state_publication_receipt::expected_prior_snapshot() const noexcept
{
  return expected_prior_snapshot_;
}

const installed_state_snapshot_identity&
state_publication_receipt::actual_prior_snapshot() const noexcept
{
  return actual_prior_snapshot_;
}

const state_target_binding&
state_publication_receipt::target_binding() const noexcept
{
  return target_binding_;
}

const state_store_identity&
state_publication_receipt::state_store() const noexcept
{
  return target_binding_.state_store();
}

const state_backend_identity&
state_publication_receipt::backend() const noexcept
{
  return target_binding_.state_backend();
}

const std::string&
state_publication_receipt::storage_format() const noexcept
{
  return storage_format_;
}

state_publication_outcome
state_publication_receipt::outcome() const noexcept
{
  return outcome_;
}

state_publication_durability
state_publication_receipt::durability() const noexcept
{
  return durability_;
}

state_storage_atomicity_boundary
state_publication_receipt::atomicity_boundary() const noexcept
{
  return atomicity_boundary_;
}

const std::optional<installed_state_snapshot_identity>&
state_publication_receipt::resulting_snapshot() const noexcept
{
  return resulting_snapshot_;
}

const std::vector<state_publication_evidence_identity>&
state_publication_receipt::subordinate_evidence() const noexcept
{
  return subordinate_evidence_;
}

bool
operator==(const state_publication_receipt& lhs,
           const state_publication_receipt& rhs) noexcept
{
  return lhs.identity_ == rhs.identity_ &&
         lhs.request_ == rhs.request_ &&
         lhs.expected_prior_snapshot_ == rhs.expected_prior_snapshot_ &&
         lhs.actual_prior_snapshot_ == rhs.actual_prior_snapshot_ &&
         lhs.target_binding_ == rhs.target_binding_ &&
         lhs.storage_format_ == rhs.storage_format_ &&
         lhs.outcome_ == rhs.outcome_ &&
         lhs.durability_ == rhs.durability_ &&
         lhs.atomicity_boundary_ == rhs.atomicity_boundary_ &&
         lhs.resulting_snapshot_ == rhs.resulting_snapshot_ &&
         lhs.subordinate_evidence_ == rhs.subordinate_evidence_;
}

bool
operator!=(const state_publication_receipt& lhs,
           const state_publication_receipt& rhs) noexcept
{
  return !(lhs == rhs);
}

bool
operator<(const state_publication_receipt& lhs,
          const state_publication_receipt& rhs) noexcept
{
  return std::tie(lhs.request_, lhs.actual_prior_snapshot_, lhs.outcome_,
                  lhs.durability_, lhs.atomicity_boundary_,
                  lhs.resulting_snapshot_, lhs.storage_format_,
                  lhs.subordinate_evidence_, lhs.identity_) <
         std::tie(rhs.request_, rhs.actual_prior_snapshot_, rhs.outcome_,
                  rhs.durability_, rhs.atomicity_boundary_,
                  rhs.resulting_snapshot_, rhs.storage_format_,
                  rhs.subordinate_evidence_, rhs.identity_);
}

} // namespace pkgstate
