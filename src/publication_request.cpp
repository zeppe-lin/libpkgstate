// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/publication_request.h>

#include "canonical_record.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <tuple>
#include <utility>

#include <libpkgstate/error.h>
#include <libpkgstate/installed_control.h>

namespace pkgstate {
namespace {

void
validate_package_name(std::string_view name)
{
  if (name.empty())
    throw state_error("publication delta package name must not be empty");

  for (const char byte : name)
  {
    if (byte == '\0' || byte == '\n' || byte == '\r')
      throw state_error("publication delta package name is not line-safe");
  }
}

bool
has_provenance(const installed_control& control,
               control_provenance_kind kind,
               std::string_view identity)
{
  const auto& provenance = control.provenance();
  return std::any_of(
      provenance.begin(), provenance.end(),
      [kind, identity](const control_provenance& reference) {
        return reference.kind() == kind && reference.identity() == identity;
      });
}

void
validate_application_evidence(
    const installed_package& proposed,
    const application_evidence_identity& application_evidence)
{
  if (!has_provenance(proposed.control(),
                      control_provenance_kind::application_evidence,
                      application_evidence.string()))
  {
    throw state_error(
        "proposed installed control does not retain matching application "
        "evidence for package " + proposed.release().name());
  }
}

void
validate_transaction_evidence(
    const installed_package& proposed,
    const transaction_evidence_identity& transaction_evidence)
{
  if (!has_provenance(proposed.control(),
                      control_provenance_kind::transaction_evidence,
                      transaction_evidence.string()))
  {
    throw state_error(
        "proposed installed control does not retain matching transaction "
        "evidence for package " + proposed.release().name());
  }
}

std::uint8_t
canonical_delta_kind(package_state_delta_kind kind)
{
  switch (kind)
  {
    case package_state_delta_kind::install:
      return 1;
    case package_state_delta_kind::replace:
      return 2;
    case package_state_delta_kind::remove:
      return 3;
  }
  throw state_error("invalid package-state delta kind");
}

void
validate_against_snapshot(const snapshot& expected,
                          const package_state_delta& delta)
{
  const installed_package* current =
      expected.find_package(delta.package_name());

  switch (delta.kind())
  {
    case package_state_delta_kind::install:
      if (current != nullptr)
      {
        throw state_error("install delta requires package absence: " +
                          delta.package_name());
      }
      break;

    case package_state_delta_kind::replace:
    case package_state_delta_kind::remove:
      if (current == nullptr)
      {
        throw state_error("publication delta requires installed package: " +
                          delta.package_name());
      }
      if (!delta.expected_package().has_value() ||
          current->identity() != *delta.expected_package())
      {
        throw state_error("publication delta old package does not match "
                          "expected snapshot: " + delta.package_name());
      }
      break;
  }

  if (delta.proposed_package().has_value())
  {
    const installed_package& proposed = *delta.proposed_package();
    if (proposed.release().name() != delta.package_name())
    {
      throw state_error("proposed installed package name does not match delta");
    }
    if (proposed.target_binding() != expected.target_binding())
    {
      throw state_error(
          "proposed installed package belongs to another target: " +
          delta.package_name());
    }
  }
}

state_publication_request_identity
identify_request(
    const snapshot& expected,
    const std::vector<package_state_delta>& deltas,
    const std::optional<transaction_evidence_identity>& transaction_evidence)
{
  detail::canonical_record record(
      state_publication_request_identity::canonical_domain());
  record.append_u16(state_publication_request_schema_version);
  record.append_digest(expected.identity());
  record.append_digest(expected.target_binding().identity());
  record.append_u64(static_cast<std::uint64_t>(deltas.size()));

  for (const package_state_delta& delta : deltas)
  {
    record.append_u8(canonical_delta_kind(delta.kind()));
    record.append_bytes(delta.package_name());
    record.append_digest(delta.operation_plan());
    record.append_digest(delta.application_evidence());

    record.append_bool(delta.expected_package().has_value());
    if (delta.expected_package().has_value())
      record.append_digest(*delta.expected_package());

    record.append_bool(delta.proposed_package().has_value());
    if (delta.proposed_package().has_value())
      record.append_digest(delta.proposed_package()->identity());
  }

  record.append_bool(transaction_evidence.has_value());
  if (transaction_evidence.has_value())
    record.append_digest(*transaction_evidence);

  return state_publication_request_identity::from_sha256(record.sha256());
}

} // namespace

package_state_delta
package_state_delta::install(
    installed_package proposed,
    operation_plan_identity operation_plan,
    application_evidence_identity application_evidence)
{
  validate_application_evidence(proposed, application_evidence);
  const std::string package_name = proposed.release().name();
  return package_state_delta(package_state_delta_kind::install,
                             package_name,
                             std::nullopt,
                             std::move(proposed),
                             std::move(operation_plan),
                             std::move(application_evidence));
}

package_state_delta
package_state_delta::replace(
    installed_package_identity expected_package,
    installed_package proposed,
    operation_plan_identity operation_plan,
    application_evidence_identity application_evidence)
{
  validate_application_evidence(proposed, application_evidence);
  if (expected_package == proposed.identity())
    throw state_error("replacement delta does not change installed package");

  const std::string package_name = proposed.release().name();
  return package_state_delta(package_state_delta_kind::replace,
                             package_name,
                             std::move(expected_package),
                             std::move(proposed),
                             std::move(operation_plan),
                             std::move(application_evidence));
}

package_state_delta
package_state_delta::remove(
    std::string_view package_name,
    installed_package_identity expected_package,
    operation_plan_identity operation_plan,
    application_evidence_identity application_evidence)
{
  validate_package_name(package_name);
  return package_state_delta(package_state_delta_kind::remove,
                             std::string(package_name),
                             std::move(expected_package),
                             std::nullopt,
                             std::move(operation_plan),
                             std::move(application_evidence));
}

package_state_delta::package_state_delta(
    package_state_delta_kind kind,
    std::string package_name,
    std::optional<installed_package_identity> expected_package,
    std::optional<installed_package> proposed_package,
    operation_plan_identity operation_plan,
    application_evidence_identity application_evidence)
    : kind_(kind),
      package_name_(std::move(package_name)),
      expected_package_(std::move(expected_package)),
      proposed_package_(std::move(proposed_package)),
      operation_plan_(std::move(operation_plan)),
      application_evidence_(std::move(application_evidence))
{
}

package_state_delta_kind
package_state_delta::kind() const noexcept
{
  return kind_;
}

const std::string&
package_state_delta::package_name() const noexcept
{
  return package_name_;
}

const std::optional<installed_package_identity>&
package_state_delta::expected_package() const noexcept
{
  return expected_package_;
}

const std::optional<installed_package>&
package_state_delta::proposed_package() const noexcept
{
  return proposed_package_;
}

const operation_plan_identity&
package_state_delta::operation_plan() const noexcept
{
  return operation_plan_;
}

const application_evidence_identity&
package_state_delta::application_evidence() const noexcept
{
  return application_evidence_;
}

bool
operator==(const package_state_delta& lhs,
           const package_state_delta& rhs) noexcept
{
  return lhs.kind_ == rhs.kind_ &&
         lhs.package_name_ == rhs.package_name_ &&
         lhs.expected_package_ == rhs.expected_package_ &&
         lhs.proposed_package_ == rhs.proposed_package_ &&
         lhs.operation_plan_ == rhs.operation_plan_ &&
         lhs.application_evidence_ == rhs.application_evidence_;
}

bool
operator!=(const package_state_delta& lhs,
           const package_state_delta& rhs) noexcept
{
  return !(lhs == rhs);
}

bool
operator<(const package_state_delta& lhs,
          const package_state_delta& rhs) noexcept
{
  return std::tie(lhs.package_name_,
                  lhs.kind_,
                  lhs.expected_package_,
                  lhs.proposed_package_,
                  lhs.operation_plan_,
                  lhs.application_evidence_) <
         std::tie(rhs.package_name_,
                  rhs.kind_,
                  rhs.expected_package_,
                  rhs.proposed_package_,
                  rhs.operation_plan_,
                  rhs.application_evidence_);
}

state_publication_request
state_publication_request::make(
    const snapshot& expected_snapshot,
    std::vector<package_state_delta> deltas,
    std::optional<transaction_evidence_identity> transaction_evidence)
{
  if (deltas.empty())
    throw state_error("state-publication request requires at least one delta");

  std::sort(deltas.begin(), deltas.end(),
            [](const package_state_delta& lhs,
               const package_state_delta& rhs) {
              return lhs.package_name() < rhs.package_name();
            });

  const auto duplicate = std::adjacent_find(
      deltas.begin(), deltas.end(),
      [](const package_state_delta& lhs,
         const package_state_delta& rhs) {
        return lhs.package_name() == rhs.package_name();
      });
  if (duplicate != deltas.end())
  {
    throw state_error("conflicting publication deltas for package: " +
                      duplicate->package_name());
  }

  if (deltas.size() > 1 && !transaction_evidence.has_value())
  {
    throw state_error(
        "composed state-publication request requires transaction evidence");
  }

  for (const package_state_delta& delta : deltas)
  {
    validate_against_snapshot(expected_snapshot, delta);
    if (transaction_evidence.has_value() &&
        delta.proposed_package().has_value())
    {
      validate_transaction_evidence(*delta.proposed_package(),
                                    *transaction_evidence);
    }
  }

  state_publication_request_identity identity = identify_request(
      expected_snapshot, deltas, transaction_evidence);

  return state_publication_request(
      std::move(identity),
      expected_snapshot.identity(),
      expected_snapshot.target_binding(),
      std::move(deltas),
      std::move(transaction_evidence));
}

state_publication_request::state_publication_request(
    state_publication_request_identity identity,
    installed_state_snapshot_identity expected_snapshot,
    state_target_binding target_binding,
    std::vector<package_state_delta> deltas,
    std::optional<transaction_evidence_identity> transaction_evidence)
    : identity_(std::move(identity)),
      expected_snapshot_(std::move(expected_snapshot)),
      target_binding_(std::move(target_binding)),
      deltas_(std::move(deltas)),
      transaction_evidence_(std::move(transaction_evidence))
{
}

std::uint16_t
state_publication_request::schema_version() const noexcept
{
  return state_publication_request_schema_version;
}

const state_publication_request_identity&
state_publication_request::identity() const noexcept
{
  return identity_;
}

const installed_state_snapshot_identity&
state_publication_request::expected_snapshot() const noexcept
{
  return expected_snapshot_;
}

const state_target_binding&
state_publication_request::target_binding() const noexcept
{
  return target_binding_;
}

const std::vector<package_state_delta>&
state_publication_request::deltas() const noexcept
{
  return deltas_;
}

const std::optional<transaction_evidence_identity>&
state_publication_request::transaction_evidence() const noexcept
{
  return transaction_evidence_;
}

} // namespace pkgstate
