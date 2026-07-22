// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/canonical_store.h>

#include "publication_projection.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include <libpkgstate/error.h>

namespace pkgstate {
namespace {

std::vector<state_publication_evidence_identity>
normalize_evidence(
    std::vector<state_publication_evidence_identity> evidence)
{
  std::sort(evidence.begin(), evidence.end());
  if (std::adjacent_find(evidence.begin(), evidence.end()) != evidence.end())
  {
    throw state_error(
        "backend publication result contains duplicate evidence");
  }
  return evidence;
}

void
validate_storage_format(std::string_view storage_format)
{
  if (storage_format.empty())
    throw state_error("canonical store storage format must not be empty");

  for (const char byte : storage_format)
  {
    if (byte == '\0' || byte == '\n' || byte == '\r')
      throw state_error("canonical store storage format must be line-safe");
  }
}

void
require_boundary(state_storage_atomicity_boundary atomicity)
{
  if (atomicity == state_storage_atomicity_boundary::none)
  {
    throw state_error(
        "backend publication result requires an atomicity boundary");
  }
}

} // namespace

state_publication_backend_result::state_publication_backend_result(
    state_publication_outcome outcome,
    state_storage_atomicity_boundary atomicity,
    bool resulting_snapshot_established,
    std::vector<state_publication_evidence_identity> evidence)
    : outcome_(outcome),
      atomicity_(atomicity),
      resulting_snapshot_established_(resulting_snapshot_established),
      evidence_(normalize_evidence(std::move(evidence)))
{
}

state_publication_backend_result
state_publication_backend_result::published(
    state_storage_atomicity_boundary atomicity,
    std::vector<state_publication_evidence_identity> evidence)
{
  require_boundary(atomicity);
  return state_publication_backend_result(
      state_publication_outcome::published, atomicity, true,
      std::move(evidence));
}

state_publication_backend_result
state_publication_backend_result::request_rejected(
    std::vector<state_publication_evidence_identity> evidence)
{
  return state_publication_backend_result(
      state_publication_outcome::request_rejected,
      state_storage_atomicity_boundary::none, false,
      std::move(evidence));
}

state_publication_backend_result
state_publication_backend_result::failed_before_publication(
    std::vector<state_publication_evidence_identity> evidence)
{
  return state_publication_backend_result(
      state_publication_outcome::failed_before_publication,
      state_storage_atomicity_boundary::none, false,
      std::move(evidence));
}

state_publication_backend_result
state_publication_backend_result::published_but_durability_unconfirmed(
    state_storage_atomicity_boundary atomicity,
    std::vector<state_publication_evidence_identity> evidence)
{
  require_boundary(atomicity);
  return state_publication_backend_result(
      state_publication_outcome::published_durability_unconfirmed,
      atomicity, true, std::move(evidence));
}

state_publication_backend_result
state_publication_backend_result::indeterminate(
    state_storage_atomicity_boundary atomicity,
    bool resulting_snapshot_established,
    std::vector<state_publication_evidence_identity> evidence)
{
  require_boundary(atomicity);
  return state_publication_backend_result(
      state_publication_outcome::indeterminate, atomicity,
      resulting_snapshot_established, std::move(evidence));
}

state_publication_outcome
state_publication_backend_result::outcome() const noexcept
{
  return outcome_;
}

state_storage_atomicity_boundary
state_publication_backend_result::atomicity_boundary() const noexcept
{
  return atomicity_;
}

bool
state_publication_backend_result::
resulting_snapshot_established() const noexcept
{
  return resulting_snapshot_established_;
}

const std::vector<state_publication_evidence_identity>&
state_publication_backend_result::subordinate_evidence() const noexcept
{
  return evidence_;
}

state_publication_receipt
canonical_store::compare_and_publish(
    const state_publication_request& request) const
{
  std::unique_ptr<canonical_publication_transaction> transaction =
      begin_publication();
  if (!transaction)
    throw store_error("canonical store returned no publication transaction");

  const snapshot actual_prior = transaction->current();
  const std::string storage_format = transaction->storage_format();
  validate_storage_format(storage_format);

  if (actual_prior.target_binding() != request.target_binding())
    throw store_error("canonical store returned state for another target");

  if (actual_prior.identity() != request.expected_snapshot())
  {
    return state_publication_receipt::stale_expected_state(
        request, actual_prior, storage_format);
  }

  const snapshot resulting_snapshot =
      detail::project_publication_request(request, actual_prior);
  const state_publication_backend_result result =
      transaction->publish(resulting_snapshot);

  switch (result.outcome())
  {
    case state_publication_outcome::published:
      return state_publication_receipt::published(
          request, actual_prior, resulting_snapshot, storage_format,
          result.atomicity_boundary(), result.subordinate_evidence());

    case state_publication_outcome::request_rejected:
      return state_publication_receipt::request_rejected(
          request, actual_prior, storage_format,
          result.subordinate_evidence());

    case state_publication_outcome::failed_before_publication:
      return state_publication_receipt::failed_before_publication(
          request, actual_prior, storage_format,
          result.subordinate_evidence());

    case state_publication_outcome::published_durability_unconfirmed:
      return state_publication_receipt::
          published_but_durability_unconfirmed(
              request, actual_prior, resulting_snapshot, storage_format,
              result.atomicity_boundary(), result.subordinate_evidence());

    case state_publication_outcome::indeterminate:
    {
      std::optional<installed_state_snapshot_identity> established;
      if (result.resulting_snapshot_established())
        established = resulting_snapshot.identity();
      return state_publication_receipt::indeterminate(
          request, actual_prior, std::move(established), storage_format,
          result.atomicity_boundary(), result.subordinate_evidence());
    }

    case state_publication_outcome::stale_expected_state:
      break;
  }

  throw state_error("backend returned an invalid publication result");
}

} // namespace pkgstate
