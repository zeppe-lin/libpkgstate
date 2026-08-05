// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/installed_control.h>

#include "canonical_record.h"

#include <tuple>
#include <utility>

#include <libpkgstate/error.h>

namespace pkgstate {
namespace {

bool line_safe(const std::string& value)
{
  if (value.empty())
    return false;
  for (const unsigned char byte : value)
    if (byte == 0 || byte == '\n' || byte == '\r' || byte < 0x20 || byte == 0x7f)
      return false;
  return true;
}

void validate_reason(installation_reason_kind kind,
                     const std::optional<package_reference>& package,
                     const std::optional<profile_reference>& profile,
                     const std::optional<source_profile_identity>& profile_identity,
                     const std::optional<std::string>& policy)
{
  const bool package_shape = package.has_value() && !profile && !profile_identity && !policy;
  const bool profile_shape = !package && profile.has_value() && profile_identity.has_value() && !policy;
  const bool policy_shape = !package && !profile && !profile_identity && policy.has_value();
  const bool empty_shape = !package && !profile && !profile_identity && !policy;
  switch (kind)
  {
    case installation_reason_kind::explicit_request:
      if (empty_shape) return;
      break;
    case installation_reason_kind::runtime_dependency:
      if (package_shape) return;
      break;
    case installation_reason_kind::profile_membership:
      if (profile_shape) return;
      break;
    case installation_reason_kind::system_policy:
      if (policy_shape && line_safe(*policy)) return;
      break;
  }
  throw state_error("invalid installation reason shape");
}

installed_control_identity identify(const package_source_record& source,
                                    const installation_reason& reason,
                                    const build_provenance& build)
{
  detail::canonical_record record(installed_control_identity::canonical_domain());
  record.append_digest(source.identity());
  record.append_u8(static_cast<std::uint8_t>(reason.kind()));
  record.append_bool(reason.issuer_package().has_value());
  if (reason.issuer_package())
    record.append_bytes(reason.issuer_package()->name());
  record.append_bool(reason.issuer_profile().has_value());
  if (reason.issuer_profile())
  {
    record.append_bytes(reason.issuer_profile()->name());
    record.append_digest(*reason.issuer_profile_identity());
  }
  record.append_bool(reason.policy().has_value());
  if (reason.policy())
    record.append_bytes(*reason.policy());
  record.append_digest(build.source_record());
  record.append_digest(build.request());
  record.append_digest(build.build_inputs());
  record.append_digest(build.environment_policy());
  record.append_digest(build.build_policy());
  record.append_digest(build.build_result());
  record.append_digest(build.payload_manifest());
  record.append_digest(build.artifact());
  record.append_digest(build.artifact_content());
  record.append_digest(build.artifact_binding());
  record.append_digest(build.execution_evidence());
  record.append_digest(build.build_image());
  record.append_digest(build.artifact_image());
  record.append_digest(build.artifact_inspection());
  return installed_control_identity::from_sha256(record.sha256());
}

} // namespace

installation_reason installation_reason::explicit_request()
{
  return installation_reason(installation_reason_kind::explicit_request,
                             std::nullopt, std::nullopt, std::nullopt, std::nullopt);
}
installation_reason installation_reason::runtime_dependency(package_reference issuer)
{
  return installation_reason(installation_reason_kind::runtime_dependency,
                             std::move(issuer), std::nullopt, std::nullopt, std::nullopt);
}
installation_reason installation_reason::profile_membership(
    profile_reference profile, source_profile_identity identity)
{
  return installation_reason(installation_reason_kind::profile_membership,
                             std::nullopt, std::move(profile), std::move(identity), std::nullopt);
}
installation_reason installation_reason::system_policy(std::string policy)
{
  return installation_reason(installation_reason_kind::system_policy,
                             std::nullopt, std::nullopt, std::nullopt, std::move(policy));
}
installation_reason::installation_reason(
    installation_reason_kind kind,
    std::optional<package_reference> issuer_package,
    std::optional<profile_reference> issuer_profile,
    std::optional<source_profile_identity> issuer_profile_identity,
    std::optional<std::string> policy)
    : kind_(kind), issuer_package_(std::move(issuer_package)),
      issuer_profile_(std::move(issuer_profile)),
      issuer_profile_identity_(std::move(issuer_profile_identity)),
      policy_(std::move(policy))
{
  validate_reason(kind_, issuer_package_, issuer_profile_, issuer_profile_identity_, policy_);
}
installation_reason_kind installation_reason::kind() const noexcept { return kind_; }
const std::optional<package_reference>& installation_reason::issuer_package() const noexcept { return issuer_package_; }
const std::optional<profile_reference>& installation_reason::issuer_profile() const noexcept { return issuer_profile_; }
const std::optional<source_profile_identity>& installation_reason::issuer_profile_identity() const noexcept { return issuer_profile_identity_; }
const std::optional<std::string>& installation_reason::policy() const noexcept { return policy_; }
bool operator==(const installation_reason& lhs, const installation_reason& rhs) noexcept { return std::tie(lhs.kind_, lhs.issuer_package_, lhs.issuer_profile_, lhs.issuer_profile_identity_, lhs.policy_) == std::tie(rhs.kind_, rhs.issuer_package_, rhs.issuer_profile_, rhs.issuer_profile_identity_, rhs.policy_); }
bool operator!=(const installation_reason& lhs, const installation_reason& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const installation_reason& lhs, const installation_reason& rhs) noexcept { return std::tie(lhs.kind_, lhs.issuer_package_, lhs.issuer_profile_, lhs.issuer_profile_identity_, lhs.policy_) < std::tie(rhs.kind_, rhs.issuer_package_, rhs.issuer_profile_, rhs.issuer_profile_identity_, rhs.policy_); }

build_provenance::build_provenance(
    package_source_record_identity source_record,
    build_request_identity request,
    build_input_set_identity build_inputs,
    environment_policy_identity environment_policy,
    build_policy_identity build_policy,
    build_result_identity build_result,
    payload_manifest_identity payload_manifest,
    build_artifact_identity artifact,
    artifact_content_identity artifact_content,
    artifact_binding_identity artifact_binding,
    execution_evidence_identity execution_evidence,
    build_image_identity build_image,
    artifact_image_identity artifact_image,
    artifact_inspection_identity artifact_inspection)
    : source_record_(std::move(source_record)),
      request_(std::move(request)),
      build_inputs_(std::move(build_inputs)),
      environment_policy_(std::move(environment_policy)),
      build_policy_(std::move(build_policy)),
      build_result_(std::move(build_result)),
      payload_manifest_(std::move(payload_manifest)),
      artifact_(std::move(artifact)),
      artifact_content_(std::move(artifact_content)),
      artifact_binding_(std::move(artifact_binding)),
      execution_evidence_(std::move(execution_evidence)),
      build_image_(std::move(build_image)),
      artifact_image_(std::move(artifact_image)),
      artifact_inspection_(std::move(artifact_inspection))
{
}
const package_source_record_identity& build_provenance::source_record() const noexcept { return source_record_; }
const build_request_identity& build_provenance::request() const noexcept { return request_; }
const build_input_set_identity& build_provenance::build_inputs() const noexcept { return build_inputs_; }
const environment_policy_identity& build_provenance::environment_policy() const noexcept { return environment_policy_; }
const build_policy_identity& build_provenance::build_policy() const noexcept { return build_policy_; }
const build_result_identity& build_provenance::build_result() const noexcept { return build_result_; }
const payload_manifest_identity& build_provenance::payload_manifest() const noexcept { return payload_manifest_; }
const build_artifact_identity& build_provenance::artifact() const noexcept { return artifact_; }
const artifact_content_identity& build_provenance::artifact_content() const noexcept { return artifact_content_; }
const artifact_binding_identity& build_provenance::artifact_binding() const noexcept { return artifact_binding_; }
const execution_evidence_identity& build_provenance::execution_evidence() const noexcept { return execution_evidence_; }
const build_image_identity& build_provenance::build_image() const noexcept { return build_image_; }
const artifact_image_identity& build_provenance::artifact_image() const noexcept { return artifact_image_; }
const artifact_inspection_identity& build_provenance::artifact_inspection() const noexcept { return artifact_inspection_; }
bool operator==(const build_provenance& lhs, const build_provenance& rhs) noexcept { return std::tie(lhs.source_record_, lhs.request_, lhs.build_inputs_, lhs.environment_policy_, lhs.build_policy_, lhs.build_result_, lhs.payload_manifest_, lhs.artifact_, lhs.artifact_content_, lhs.artifact_binding_, lhs.execution_evidence_, lhs.build_image_, lhs.artifact_image_, lhs.artifact_inspection_) == std::tie(rhs.source_record_, rhs.request_, rhs.build_inputs_, rhs.environment_policy_, rhs.build_policy_, rhs.build_result_, rhs.payload_manifest_, rhs.artifact_, rhs.artifact_content_, rhs.artifact_binding_, rhs.execution_evidence_, rhs.build_image_, rhs.artifact_image_, rhs.artifact_inspection_); }
bool operator!=(const build_provenance& lhs, const build_provenance& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const build_provenance& lhs, const build_provenance& rhs) noexcept { return std::tie(lhs.source_record_, lhs.request_, lhs.build_inputs_, lhs.environment_policy_, lhs.build_policy_, lhs.build_result_, lhs.payload_manifest_, lhs.artifact_, lhs.artifact_content_, lhs.artifact_binding_, lhs.execution_evidence_, lhs.build_image_, lhs.artifact_image_, lhs.artifact_inspection_) < std::tie(rhs.source_record_, rhs.request_, rhs.build_inputs_, rhs.environment_policy_, rhs.build_policy_, rhs.build_result_, rhs.payload_manifest_, rhs.artifact_, rhs.artifact_content_, rhs.artifact_binding_, rhs.execution_evidence_, rhs.build_image_, rhs.artifact_image_, rhs.artifact_inspection_); }

installed_control installed_control::make(
    package_source_record source,
    installation_reason reason,
    build_provenance build)
{
  if (build.source_record() != source.identity())
    throw state_error("build provenance does not bind the package source record");
  installed_control_identity identity = identify(source, reason, build);
  return installed_control(std::move(identity), std::move(source),
                           std::move(reason), std::move(build));
}
installed_control::installed_control(installed_control_identity identity,
                                     package_source_record source,
                                     installation_reason reason,
                                     build_provenance build)
    : identity_(std::move(identity)), source_(std::move(source)),
      reason_(std::move(reason)), build_(std::move(build))
{
}
const installed_control_identity& installed_control::identity() const noexcept { return identity_; }
const package_source_record& installed_control::source() const noexcept { return source_; }
const package_release& installed_control::release() const noexcept { return source_.release(); }
const installation_reason& installed_control::reason() const noexcept { return reason_; }
const build_provenance& installed_control::build() const noexcept { return build_; }
bool operator==(const installed_control& lhs, const installed_control& rhs) noexcept { return lhs.identity_ == rhs.identity_ && std::tie(lhs.source_, lhs.reason_, lhs.build_) == std::tie(rhs.source_, rhs.reason_, rhs.build_); }
bool operator!=(const installed_control& lhs, const installed_control& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const installed_control& lhs, const installed_control& rhs) noexcept { return lhs.identity_ < rhs.identity_; }

} // namespace pkgstate
