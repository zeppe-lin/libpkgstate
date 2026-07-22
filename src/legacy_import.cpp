// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/legacy_import.h>

#include "canonical_record.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <libpkgstate/canonical_store.h>
#include <libpkgstate/error.h>

namespace pkgstate {
namespace {

constexpr std::array<control_provenance_kind, 5> importable_provenance = {
    control_provenance_kind::candidate_control,
    control_provenance_kind::artifact,
    control_provenance_kind::artifact_manifest,
    control_provenance_kind::application_evidence,
    control_provenance_kind::transaction_evidence,
};

bool
is_importable_provenance(control_provenance_kind kind) noexcept
{
  return std::find(importable_provenance.begin(), importable_provenance.end(),
                   kind) != importable_provenance.end();
}

void
validate_import_state(installed_control_fact_state state,
                      bool values_empty,
                      const char* label)
{
  switch (state)
  {
    case installed_control_fact_state::supplied_by_migration:
      return;
    case installed_control_fact_state::historically_unavailable:
      if (!values_empty)
      {
        throw state_error(std::string(label) +
                          " cannot contain values when historically "
                          "unavailable");
      }
      return;
    case installed_control_fact_state::recorded_at_installation:
    case installed_control_fact_state::recorded_in_compatibility_storage:
      break;
  }
  throw state_error(std::string(label) +
                    " must be supplied by migration or historically "
                    "unavailable");
}

void
validate_storage_format(const std::optional<std::string>& storage_format,
                        bool required)
{
  if (!storage_format)
  {
    if (required)
      throw state_error("legacy import receipt requires a storage format");
    return;
  }
  if (storage_format->empty())
    throw state_error("legacy import storage format must not be empty");
  for (const char byte : *storage_format)
  {
    if (byte == '\0' || byte == '\n' || byte == '\r')
      throw state_error("legacy import storage format must be line-safe");
  }
}

std::vector<state_publication_evidence_identity>
normalize_evidence(std::vector<state_publication_evidence_identity> evidence)
{
  std::sort(evidence.begin(), evidence.end());
  if (std::adjacent_find(evidence.begin(), evidence.end()) != evidence.end())
    throw state_error("legacy import receipt contains duplicate evidence");
  return evidence;
}

std::vector<runtime_dependency_declaration>
normalize_dependencies(std::vector<runtime_dependency_declaration> values)
{
  std::sort(values.begin(), values.end());
  if (std::adjacent_find(values.begin(), values.end()) != values.end())
    throw state_error("legacy import contains duplicate runtime dependency");
  return values;
}

std::vector<removal_lifecycle_declaration>
normalize_lifecycle(std::vector<removal_lifecycle_declaration> values)
{
  std::stable_sort(values.begin(), values.end(),
                   [](const auto& lhs, const auto& rhs) {
                     return lhs.phase() < rhs.phase();
                   });
  return values;
}

std::vector<target_profile_fact>
normalize_profile(std::vector<target_profile_fact> values)
{
  std::sort(values.begin(), values.end());
  const auto duplicate = std::adjacent_find(
      values.begin(), values.end(),
      [](const auto& lhs, const auto& rhs) {
        return lhs.name() == rhs.name();
      });
  if (duplicate != values.end())
    throw state_error("legacy import contains duplicate target profile fact");
  return values;
}

std::vector<legacy_import_provenance>
normalize_provenance(std::vector<legacy_import_provenance> values)
{
  std::sort(values.begin(), values.end());
  const auto duplicate = std::adjacent_find(
      values.begin(), values.end(),
      [](const auto& lhs, const auto& rhs) {
        return lhs.kind() == rhs.kind();
      });
  if (duplicate != values.end())
    throw state_error("legacy import contains duplicate provenance decision");
  if (values.size() != importable_provenance.size())
    throw state_error(
        "legacy import requires one decision for every provenance class");
  for (std::size_t index = 0; index < importable_provenance.size(); ++index)
  {
    if (values[index].kind() != importable_provenance[index])
      throw state_error("legacy import omits a provenance decision");
  }
  return values;
}

void
append_provenance(detail::canonical_record& record,
                  const legacy_import_provenance& provenance)
{
  record.append_u8(static_cast<std::uint8_t>(provenance.kind()));
  record.append_u8(static_cast<std::uint8_t>(provenance.state()));
  record.append_bool(provenance.identity().has_value());
  if (provenance.identity())
    record.append_bytes(*provenance.identity());
}

void
append_package_import(detail::canonical_record& record,
                      const legacy_package_import& package)
{
  record.append_bytes(package.package_name());
  record.append_digest(package.source_package());
  record.append_digest(package.release().identity());

  record.append_u8(static_cast<std::uint8_t>(
      package.runtime_dependencies_state()));
  record.append_u64(static_cast<std::uint64_t>(
      package.runtime_dependencies().size()));
  for (const auto& dependency : package.runtime_dependencies())
    record.append_bytes(dependency.expression());

  record.append_u8(static_cast<std::uint8_t>(
      package.removal_lifecycle_state()));
  record.append_u64(static_cast<std::uint64_t>(
      package.removal_lifecycle().size()));
  for (const auto& declaration : package.removal_lifecycle())
  {
    record.append_u8(static_cast<std::uint8_t>(declaration.phase()));
    record.append_bytes(declaration.format());
    record.append_bytes(declaration.material());
  }

  record.append_u8(static_cast<std::uint8_t>(package.target_profile_state()));
  record.append_u64(
      static_cast<std::uint64_t>(package.target_profile().size()));
  for (const auto& fact : package.target_profile())
  {
    record.append_bytes(fact.name());
    record.append_bytes(fact.value());
  }

  record.append_u64(static_cast<std::uint64_t>(package.provenance().size()));
  for (const auto& provenance : package.provenance())
    append_provenance(record, provenance);
}

legacy_import_request_identity
identify_request(const legacy_import_source& source,
                 const legacy_snapshot_observation_identity& source_snapshot,
                 const installed_state_snapshot_identity& expected_destination,
                 const state_target_binding& destination,
                 const legacy_migration_evidence_identity& migration_evidence,
                 const std::vector<legacy_package_import>& packages,
                 const snapshot& resulting_snapshot)
{
  detail::canonical_record record(
      legacy_import_request_identity::canonical_domain());
  record.append_u16(legacy_import_request_schema_version);
  record.append_digest(source.managed_target());
  record.append_digest(source.state_store());
  record.append_digest(source.root_view());
  record.append_digest(source.state_backend());
  record.append_digest(source_snapshot);
  record.append_digest(expected_destination);
  record.append_digest(destination.identity());
  record.append_digest(migration_evidence);
  record.append_u64(static_cast<std::uint64_t>(packages.size()));
  for (const auto& package : packages)
    append_package_import(record, package);
  record.append_digest(resulting_snapshot.identity());
  return legacy_import_request_identity::from_sha256(record.sha256());
}

legacy_import_receipt_identity
identify_receipt(
    const legacy_import_request& request,
    const installed_state_snapshot_identity& actual_destination,
    legacy_import_outcome outcome,
    state_publication_durability durability,
    state_storage_atomicity_boundary atomicity,
    const std::optional<installed_state_snapshot_identity>& resulting_snapshot,
    const std::optional<std::string>& storage_format,
    const std::vector<state_publication_evidence_identity>& evidence)
{
  detail::canonical_record record(
      legacy_import_receipt_identity::canonical_domain());
  record.append_u16(legacy_import_receipt_schema_version);
  record.append_digest(request.identity());
  record.append_digest(request.source_snapshot());
  record.append_digest(request.expected_destination());
  record.append_digest(actual_destination);
  record.append_digest(request.destination_binding().identity());
  record.append_u8(static_cast<std::uint8_t>(outcome));
  record.append_u8(static_cast<std::uint8_t>(durability));
  record.append_u8(static_cast<std::uint8_t>(atomicity));
  record.append_bool(resulting_snapshot.has_value());
  if (resulting_snapshot)
    record.append_digest(*resulting_snapshot);
  record.append_bool(storage_format.has_value());
  if (storage_format)
    record.append_bytes(*storage_format);
  record.append_u64(static_cast<std::uint64_t>(evidence.size()));
  for (const auto& item : evidence)
    record.append_digest(item);
  return legacy_import_receipt_identity::from_sha256(record.sha256());
}

void
require_atomicity(state_storage_atomicity_boundary atomicity)
{
  if (atomicity == state_storage_atomicity_boundary::none)
    throw state_error(
        "legacy import publication requires an atomicity boundary");
}

} // namespace

legacy_import_source
legacy_import_source::make(managed_target_identity managed_target,
                           state_store_identity state_store,
                           root_view_identity root_view,
                           state_backend_identity state_backend)
{
  return legacy_import_source(std::move(managed_target), std::move(state_store),
                              std::move(root_view), std::move(state_backend));
}

legacy_import_source::legacy_import_source(
    managed_target_identity managed_target,
    state_store_identity state_store,
    root_view_identity root_view,
    state_backend_identity state_backend)
    : managed_target_(std::move(managed_target)),
      state_store_(std::move(state_store)),
      root_view_(std::move(root_view)),
      state_backend_(std::move(state_backend))
{
}

const managed_target_identity&
legacy_import_source::managed_target() const noexcept
{
  return managed_target_;
}
const state_store_identity&
legacy_import_source::state_store() const noexcept
{
  return state_store_;
}
const root_view_identity&
legacy_import_source::root_view() const noexcept
{
  return root_view_;
}
const state_backend_identity&
legacy_import_source::state_backend() const noexcept
{
  return state_backend_;
}

bool operator==(const legacy_import_source& lhs,
                const legacy_import_source& rhs) noexcept
{
  return std::tie(lhs.managed_target_, lhs.state_store_, lhs.root_view_,
                  lhs.state_backend_) ==
         std::tie(rhs.managed_target_, rhs.state_store_, rhs.root_view_,
                  rhs.state_backend_);
}
bool
operator!=(const legacy_import_source& lhs,
           const legacy_import_source& rhs) noexcept
{
  return !(lhs == rhs);
}
bool operator<(const legacy_import_source& lhs,
               const legacy_import_source& rhs) noexcept
{
  return std::tie(lhs.managed_target_, lhs.state_store_, lhs.root_view_,
                  lhs.state_backend_) <
         std::tie(rhs.managed_target_, rhs.state_store_, rhs.root_view_,
                  rhs.state_backend_);
}

legacy_import_provenance
legacy_import_provenance::supplied(control_provenance_kind kind,
                                   std::string_view identity)
{
  if (!is_importable_provenance(kind))
    throw state_error("legacy import provenance kind is reserved");
  const detail::digest_value parsed = detail::digest_value::parse(identity);
  return legacy_import_provenance(
      kind, installed_control_fact_state::supplied_by_migration,
      parsed.string());
}

legacy_import_provenance
legacy_import_provenance::historically_unavailable(control_provenance_kind kind)
{
  if (!is_importable_provenance(kind))
    throw state_error("legacy import provenance kind is reserved");
  return legacy_import_provenance(
      kind, installed_control_fact_state::historically_unavailable,
      std::nullopt);
}

legacy_import_provenance::legacy_import_provenance(
    control_provenance_kind kind,
    installed_control_fact_state state,
    std::optional<std::string> identity)
    : kind_(kind), state_(state), identity_(std::move(identity))
{
}

control_provenance_kind
legacy_import_provenance::kind() const noexcept { return kind_; }
installed_control_fact_state
legacy_import_provenance::state() const noexcept { return state_; }
const std::optional<std::string>&
legacy_import_provenance::identity() const noexcept { return identity_; }

bool operator==(const legacy_import_provenance& lhs,
                const legacy_import_provenance& rhs) noexcept
{
  return std::tie(lhs.kind_, lhs.state_, lhs.identity_) ==
         std::tie(rhs.kind_, rhs.state_, rhs.identity_);
}
bool
operator!=(const legacy_import_provenance& lhs,
           const legacy_import_provenance& rhs) noexcept
{
  return !(lhs == rhs);
}
bool operator<(const legacy_import_provenance& lhs,
               const legacy_import_provenance& rhs) noexcept
{
  return std::tie(lhs.kind_, lhs.state_, lhs.identity_) <
         std::tie(rhs.kind_, rhs.state_, rhs.identity_);
}

legacy_package_import
legacy_package_import::make(
    const legacy_installed_package& source,
    package_release release,
    installed_control_fact_state runtime_dependencies_state,
    std::vector<runtime_dependency_declaration> runtime_dependencies,
    installed_control_fact_state removal_lifecycle_state,
    std::vector<removal_lifecycle_declaration> removal_lifecycle,
    installed_control_fact_state target_profile_state,
    std::vector<target_profile_fact> target_profile,
    std::vector<legacy_import_provenance> provenance)
{
  if (release.name() != source.identity().name())
    throw state_error(
        "legacy import package release name does not match source");

  validate_import_state(runtime_dependencies_state,
                        runtime_dependencies.empty(), "runtime dependencies");
  validate_import_state(removal_lifecycle_state, removal_lifecycle.empty(),
                        "removal lifecycle");
  validate_import_state(target_profile_state, target_profile.empty(),
                        "target profile");

  return legacy_package_import(
      source.identity().name(), source.observation_identity(),
      std::move(release), runtime_dependencies_state,
      normalize_dependencies(std::move(runtime_dependencies)),
      removal_lifecycle_state,
      normalize_lifecycle(std::move(removal_lifecycle)),
      target_profile_state, normalize_profile(std::move(target_profile)),
      normalize_provenance(std::move(provenance)));
}

legacy_package_import::legacy_package_import(
    std::string package_name,
    legacy_package_observation_identity source_package,
    package_release release,
    installed_control_fact_state runtime_dependencies_state,
    std::vector<runtime_dependency_declaration> runtime_dependencies,
    installed_control_fact_state removal_lifecycle_state,
    std::vector<removal_lifecycle_declaration> removal_lifecycle,
    installed_control_fact_state target_profile_state,
    std::vector<target_profile_fact> target_profile,
    std::vector<legacy_import_provenance> provenance)
    : package_name_(std::move(package_name)),
      source_package_(std::move(source_package)),
      release_(std::move(release)),
      runtime_dependencies_state_(runtime_dependencies_state),
      runtime_dependencies_(std::move(runtime_dependencies)),
      removal_lifecycle_state_(removal_lifecycle_state),
      removal_lifecycle_(std::move(removal_lifecycle)),
      target_profile_state_(target_profile_state),
      target_profile_(std::move(target_profile)),
      provenance_(std::move(provenance))
{
}

const std::string&
legacy_package_import::package_name() const noexcept
{
  return package_name_;
}
const legacy_package_observation_identity&
legacy_package_import::source_package() const noexcept
{
  return source_package_;
}
const package_release&
legacy_package_import::release() const noexcept
{
  return release_;
}
installed_control_fact_state
legacy_package_import::runtime_dependencies_state() const noexcept
{
  return runtime_dependencies_state_;
}
const std::vector<runtime_dependency_declaration>&
legacy_package_import::runtime_dependencies() const noexcept
{
  return runtime_dependencies_;
}
installed_control_fact_state
legacy_package_import::removal_lifecycle_state() const noexcept
{
  return removal_lifecycle_state_;
}
const std::vector<removal_lifecycle_declaration>&
legacy_package_import::removal_lifecycle() const noexcept
{
  return removal_lifecycle_;
}
installed_control_fact_state
legacy_package_import::target_profile_state() const noexcept
{
  return target_profile_state_;
}
const std::vector<target_profile_fact>&
legacy_package_import::target_profile() const noexcept
{
  return target_profile_;
}
const std::vector<legacy_import_provenance>&
legacy_package_import::provenance() const noexcept
{
  return provenance_;
}

bool operator==(const legacy_package_import& lhs,
                const legacy_package_import& rhs) noexcept
{
  return std::tie(lhs.package_name_, lhs.source_package_, lhs.release_,
                  lhs.runtime_dependencies_state_, lhs.runtime_dependencies_,
                  lhs.removal_lifecycle_state_, lhs.removal_lifecycle_,
                  lhs.target_profile_state_, lhs.target_profile_,
                  lhs.provenance_) ==
         std::tie(rhs.package_name_, rhs.source_package_, rhs.release_,
                  rhs.runtime_dependencies_state_, rhs.runtime_dependencies_,
                  rhs.removal_lifecycle_state_, rhs.removal_lifecycle_,
                  rhs.target_profile_state_, rhs.target_profile_,
                  rhs.provenance_);
}
bool
operator!=(const legacy_package_import& lhs,
           const legacy_package_import& rhs) noexcept
{
  return !(lhs == rhs);
}
bool operator<(const legacy_package_import& lhs,
               const legacy_package_import& rhs) noexcept
{
  return lhs.package_name_ < rhs.package_name_;
}

legacy_import_request
legacy_import_request::make(
    legacy_import_source source,
    const legacy_snapshot& source_snapshot,
    const snapshot& expected_destination,
    std::vector<legacy_package_import> packages,
    legacy_migration_evidence_identity migration_evidence)
{
  if (!expected_destination.packages().empty())
    throw state_error("legacy import requires an empty canonical destination");

  const state_target_binding& destination =
      expected_destination.target_binding();
  if (source.managed_target() != destination.managed_target() ||
      source.root_view() != destination.root_view())
  {
    throw state_error(
        "legacy import source and destination identify different targets");
  }
  if (source.state_store() == destination.state_store() ||
      source.state_backend() == destination.state_backend())
  {
    throw state_error(
        "legacy import source and canonical destination must be distinct");
  }

  std::sort(packages.begin(), packages.end());
  const auto duplicate = std::adjacent_find(
      packages.begin(), packages.end(),
      [](const auto& lhs, const auto& rhs) {
        return lhs.package_name() == rhs.package_name();
      });
  if (duplicate != packages.end())
    throw state_error("legacy import contains duplicate package input");
  if (packages.size() != source_snapshot.size())
    throw state_error(
        "legacy import requires one input for every source package");

  std::vector<installed_package> installed;
  installed.reserve(packages.size());
  for (std::size_t index = 0; index < packages.size(); ++index)
  {
    const legacy_package_import& input = packages[index];
    const legacy_installed_package* source_package =
        source_snapshot.find_package(input.package_name());
    if (!source_package ||
        source_package->observation_identity() != input.source_package())
    {
      throw state_error(
          "legacy import package input does not match source observation");
    }

    std::vector<control_provenance> control_provenance_values;
    for (const legacy_import_provenance& provenance : input.provenance())
    {
      if (provenance.identity())
      {
        control_provenance_values.push_back(control_provenance::make(
            provenance.kind(), *provenance.identity()));
      }
    }
    control_provenance_values.push_back(control_provenance::make(
        control_provenance_kind::legacy_package_observation,
        source_package->observation_identity().string()));
    control_provenance_values.push_back(control_provenance::make(
        control_provenance_kind::legacy_snapshot_observation,
        source_snapshot.observation_identity().string()));
    control_provenance_values.push_back(control_provenance::make(
        control_provenance_kind::legacy_migration_evidence,
        migration_evidence.string()));

    installed_control_completeness completeness;
    completeness.runtime_dependencies = input.runtime_dependencies_state();
    completeness.removal_lifecycle = input.removal_lifecycle_state();
    completeness.target_profile = input.target_profile_state();
    completeness.provenance =
        installed_control_fact_state::supplied_by_migration;

    installed_control control = installed_control::make(
        input.release(), completeness, input.runtime_dependencies(),
        input.removal_lifecycle(), input.target_profile(),
        std::move(control_provenance_values));
    installed.push_back(installed_package::make(
        input.release(), std::move(control), destination,
        source_package->manifest()));
  }

  snapshot resulting = snapshot::make(destination, std::move(installed));
  legacy_import_request_identity identity = identify_request(
      source, source_snapshot.observation_identity(),
      expected_destination.identity(), destination, migration_evidence,
      packages, resulting);
  return legacy_import_request(
      std::move(identity), std::move(source),
      source_snapshot.observation_identity(), expected_destination.identity(),
      destination, std::move(migration_evidence), std::move(packages),
      std::move(resulting));
}

legacy_import_request::legacy_import_request(
    legacy_import_request_identity identity,
    legacy_import_source source,
    legacy_snapshot_observation_identity source_snapshot,
    installed_state_snapshot_identity expected_destination,
    state_target_binding destination_binding,
    legacy_migration_evidence_identity migration_evidence,
    std::vector<legacy_package_import> packages,
    snapshot resulting_snapshot)
    : identity_(std::move(identity)), source_(std::move(source)),
      source_snapshot_(std::move(source_snapshot)),
      expected_destination_(std::move(expected_destination)),
      destination_binding_(std::move(destination_binding)),
      migration_evidence_(std::move(migration_evidence)),
      packages_(std::move(packages)),
      resulting_snapshot_(std::move(resulting_snapshot))
{
}

std::uint16_t
legacy_import_request::schema_version() const noexcept
{
  return legacy_import_request_schema_version;
}
const legacy_import_request_identity&
legacy_import_request::identity() const noexcept
{
  return identity_;
}
const legacy_import_source&
legacy_import_request::source() const noexcept
{
  return source_;
}
const legacy_snapshot_observation_identity&
legacy_import_request::source_snapshot() const noexcept
{
  return source_snapshot_;
}
const installed_state_snapshot_identity&
legacy_import_request::expected_destination() const noexcept
{
  return expected_destination_;
}
const state_target_binding&
legacy_import_request::destination_binding() const noexcept
{
  return destination_binding_;
}
const legacy_migration_evidence_identity&
legacy_import_request::migration_evidence() const noexcept
{
  return migration_evidence_;
}
const std::vector<legacy_package_import>&
legacy_import_request::packages() const noexcept
{
  return packages_;
}
const snapshot&
legacy_import_request::resulting_snapshot() const noexcept
{
  return resulting_snapshot_;
}

legacy_import_receipt
legacy_import_receipt::make(
    const legacy_import_request& request,
    const snapshot& actual_prior,
    legacy_import_outcome outcome,
    state_publication_durability durability,
    state_storage_atomicity_boundary atomicity,
    std::optional<installed_state_snapshot_identity> resulting_snapshot,
    std::optional<std::string> storage_format,
    std::vector<state_publication_evidence_identity> evidence)
{
  if (actual_prior.target_binding() != request.destination_binding())
    throw state_error("legacy import receipt target does not match request");

  const bool actual_is_expected =
      actual_prior.identity() == request.expected_destination();
  const bool result_is_expected =
      resulting_snapshot &&
      *resulting_snapshot == request.resulting_snapshot().identity();

  switch (outcome)
  {
    case legacy_import_outcome::validated_only:
      if (!actual_is_expected || !result_is_expected || storage_format ||
          durability != state_publication_durability::not_attempted ||
          atomicity != state_storage_atomicity_boundary::none)
        throw state_error("invalid validation-only legacy import receipt");
      break;
    case legacy_import_outcome::imported:
      if (!actual_is_expected || !result_is_expected ||
          durability != state_publication_durability::confirmed)
        throw state_error("invalid successful legacy import receipt");
      require_atomicity(atomicity);
      validate_storage_format(storage_format, true);
      break;
    case legacy_import_outcome::stale_destination:
      if (actual_is_expected || resulting_snapshot ||
          durability != state_publication_durability::not_attempted ||
          atomicity != state_storage_atomicity_boundary::none)
        throw state_error("invalid stale legacy import receipt");
      validate_storage_format(storage_format, true);
      break;
    case legacy_import_outcome::request_rejected:
    case legacy_import_outcome::failed_before_publication:
      if (!actual_is_expected || resulting_snapshot ||
          durability != state_publication_durability::not_attempted ||
          atomicity != state_storage_atomicity_boundary::none)
        throw state_error("invalid prepublication legacy import receipt");
      validate_storage_format(storage_format, true);
      break;
    case legacy_import_outcome::imported_durability_unconfirmed:
      if (!actual_is_expected || !result_is_expected ||
          durability != state_publication_durability::unconfirmed)
        throw state_error("invalid durability-unconfirmed import receipt");
      require_atomicity(atomicity);
      validate_storage_format(storage_format, true);
      break;
    case legacy_import_outcome::indeterminate:
      if (!actual_is_expected ||
          (resulting_snapshot && !result_is_expected) ||
          durability != state_publication_durability::indeterminate)
        throw state_error("invalid indeterminate legacy import receipt");
      require_atomicity(atomicity);
      validate_storage_format(storage_format, true);
      break;
  }

  validate_storage_format(storage_format, false);
  evidence = normalize_evidence(std::move(evidence));
  legacy_import_receipt_identity identity = identify_receipt(
      request, actual_prior.identity(), outcome, durability, atomicity,
      resulting_snapshot, storage_format, evidence);
  return legacy_import_receipt(
      std::move(identity), request.identity(), request.source_snapshot(),
      request.expected_destination(), actual_prior.identity(),
      request.destination_binding(), outcome, durability, atomicity,
      std::move(resulting_snapshot), std::move(storage_format),
      std::move(evidence));
}

legacy_import_receipt
legacy_import_receipt::validated(
    const legacy_import_request& request)
{
  return make(request,
              snapshot::make(request.destination_binding(), {}),
              legacy_import_outcome::validated_only,
              state_publication_durability::not_attempted,
              state_storage_atomicity_boundary::none,
              request.resulting_snapshot().identity(), std::nullopt, {});
}

legacy_import_receipt legacy_import_receipt::imported(
    const legacy_import_request& request, const snapshot& actual_prior,
    std::string storage_format, state_storage_atomicity_boundary atomicity,
    std::vector<state_publication_evidence_identity> evidence)
{
  return make(request, actual_prior, legacy_import_outcome::imported,
              state_publication_durability::confirmed, atomicity,
              request.resulting_snapshot().identity(),
              std::move(storage_format), std::move(evidence));
}

legacy_import_receipt legacy_import_receipt::stale_destination(
    const legacy_import_request& request, const snapshot& actual_prior,
    std::string storage_format,
    std::vector<state_publication_evidence_identity> evidence)
{
  return make(request, actual_prior, legacy_import_outcome::stale_destination,
              state_publication_durability::not_attempted,
              state_storage_atomicity_boundary::none, std::nullopt,
              std::move(storage_format), std::move(evidence));
}

legacy_import_receipt legacy_import_receipt::request_rejected(
    const legacy_import_request& request, const snapshot& actual_prior,
    std::string storage_format,
    std::vector<state_publication_evidence_identity> evidence)
{
  return make(request, actual_prior, legacy_import_outcome::request_rejected,
              state_publication_durability::not_attempted,
              state_storage_atomicity_boundary::none, std::nullopt,
              std::move(storage_format), std::move(evidence));
}

legacy_import_receipt legacy_import_receipt::failed_before_publication(
    const legacy_import_request& request, const snapshot& actual_prior,
    std::string storage_format,
    std::vector<state_publication_evidence_identity> evidence)
{
  return make(request, actual_prior,
              legacy_import_outcome::failed_before_publication,
              state_publication_durability::not_attempted,
              state_storage_atomicity_boundary::none, std::nullopt,
              std::move(storage_format), std::move(evidence));
}

legacy_import_receipt
legacy_import_receipt::imported_but_durability_unconfirmed(
    const legacy_import_request& request, const snapshot& actual_prior,
    std::string storage_format, state_storage_atomicity_boundary atomicity,
    std::vector<state_publication_evidence_identity> evidence)
{
  return make(request, actual_prior,
              legacy_import_outcome::imported_durability_unconfirmed,
              state_publication_durability::unconfirmed, atomicity,
              request.resulting_snapshot().identity(),
              std::move(storage_format), std::move(evidence));
}

legacy_import_receipt legacy_import_receipt::indeterminate(
    const legacy_import_request& request, const snapshot& actual_prior,
    bool resulting_snapshot_established, std::string storage_format,
    state_storage_atomicity_boundary atomicity,
    std::vector<state_publication_evidence_identity> evidence)
{
  std::optional<installed_state_snapshot_identity> resulting;
  if (resulting_snapshot_established)
    resulting = request.resulting_snapshot().identity();
  return make(request, actual_prior, legacy_import_outcome::indeterminate,
              state_publication_durability::indeterminate, atomicity,
              std::move(resulting), std::move(storage_format),
              std::move(evidence));
}

std::uint16_t
legacy_import_receipt::schema_version() const noexcept
{
  return legacy_import_receipt_schema_version;
}
const legacy_import_receipt_identity&
legacy_import_receipt::identity() const noexcept
{
  return identity_;
}
const legacy_import_request_identity&
legacy_import_receipt::request() const noexcept
{
  return request_;
}
const legacy_snapshot_observation_identity&
legacy_import_receipt::source_snapshot() const noexcept
{
  return source_snapshot_;
}
const installed_state_snapshot_identity&
legacy_import_receipt::expected_destination() const noexcept
{
  return expected_destination_;
}
const installed_state_snapshot_identity&
legacy_import_receipt::actual_destination() const noexcept
{
  return actual_destination_;
}
const state_target_binding&
legacy_import_receipt::destination_binding() const noexcept
{
  return destination_binding_;
}
legacy_import_outcome
legacy_import_receipt::outcome() const noexcept
{
  return outcome_;
}
state_publication_durability
legacy_import_receipt::durability() const noexcept
{
  return durability_;
}
state_storage_atomicity_boundary
legacy_import_receipt::atomicity_boundary() const noexcept
{
  return atomicity_;
}
const std::optional<installed_state_snapshot_identity>&
legacy_import_receipt::resulting_snapshot() const noexcept
{
  return resulting_snapshot_;
}
const std::optional<std::string>&
legacy_import_receipt::storage_format() const noexcept
{
  return storage_format_;
}
const std::vector<state_publication_evidence_identity>&
legacy_import_receipt::subordinate_evidence() const noexcept
{
  return evidence_;
}

legacy_import_receipt::legacy_import_receipt(
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
    std::vector<state_publication_evidence_identity> evidence)
    : identity_(std::move(identity)), request_(std::move(request)),
      source_snapshot_(std::move(source_snapshot)),
      expected_destination_(std::move(expected_destination)),
      actual_destination_(std::move(actual_destination)),
      destination_binding_(std::move(destination_binding)), outcome_(outcome),
      durability_(durability), atomicity_(atomicity),
      resulting_snapshot_(std::move(resulting_snapshot)),
      storage_format_(std::move(storage_format)), evidence_(std::move(evidence))
{
}

legacy_import_receipt
canonical_store::import_legacy(const legacy_import_request& request) const
{
  std::unique_ptr<canonical_publication_transaction> transaction =
      begin_publication();
  if (!transaction)
    throw store_error("canonical store returned no publication transaction");

  const snapshot actual_prior = transaction->current();
  const std::string storage_format = transaction->storage_format();
  validate_storage_format(std::optional<std::string>(storage_format), true);

  if (actual_prior.target_binding() != request.destination_binding())
    throw store_error("canonical store returned state for another target");

  if (actual_prior.identity() != request.expected_destination())
  {
    return legacy_import_receipt::stale_destination(
        request, actual_prior, storage_format);
  }

  const state_publication_backend_result result =
      transaction->publish(request.resulting_snapshot());
  switch (result.outcome())
  {
    case state_publication_outcome::published:
      return legacy_import_receipt::imported(
          request, actual_prior, storage_format, result.atomicity_boundary(),
          result.subordinate_evidence());
    case state_publication_outcome::request_rejected:
      return legacy_import_receipt::request_rejected(
          request, actual_prior, storage_format, result.subordinate_evidence());
    case state_publication_outcome::failed_before_publication:
      return legacy_import_receipt::failed_before_publication(
          request, actual_prior, storage_format, result.subordinate_evidence());
    case state_publication_outcome::published_durability_unconfirmed:
      return legacy_import_receipt::imported_but_durability_unconfirmed(
          request, actual_prior, storage_format, result.atomicity_boundary(),
          result.subordinate_evidence());
    case state_publication_outcome::indeterminate:
      return legacy_import_receipt::indeterminate(
          request, actual_prior, result.resulting_snapshot_established(),
          storage_format, result.atomicity_boundary(),
          result.subordinate_evidence());
    case state_publication_outcome::stale_expected_state:
      break;
  }
  throw state_error("backend returned an invalid legacy import result");
}

} // namespace pkgstate
