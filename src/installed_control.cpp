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
  record.append_digest(build.candidate_control());
  record.append_digest(build.build_inputs());
  record.append_digest(build.build_result());
  record.append_digest(build.artifact());
  record.append_digest(build.artifact_manifest());
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
    candidate_control_identity candidate_control,
    build_input_set_identity build_inputs,
    build_result_identity build_result,
    artifact_identity artifact,
    artifact_manifest_identity artifact_manifest)
    : candidate_control_(std::move(candidate_control)),
      build_inputs_(std::move(build_inputs)),
      build_result_(std::move(build_result)), artifact_(std::move(artifact)),
      artifact_manifest_(std::move(artifact_manifest))
{
}
const candidate_control_identity& build_provenance::candidate_control() const noexcept { return candidate_control_; }
const build_input_set_identity& build_provenance::build_inputs() const noexcept { return build_inputs_; }
const build_result_identity& build_provenance::build_result() const noexcept { return build_result_; }
const artifact_identity& build_provenance::artifact() const noexcept { return artifact_; }
const artifact_manifest_identity& build_provenance::artifact_manifest() const noexcept { return artifact_manifest_; }
bool operator==(const build_provenance& lhs, const build_provenance& rhs) noexcept { return std::tie(lhs.candidate_control_, lhs.build_inputs_, lhs.build_result_, lhs.artifact_, lhs.artifact_manifest_) == std::tie(rhs.candidate_control_, rhs.build_inputs_, rhs.build_result_, rhs.artifact_, rhs.artifact_manifest_); }
bool operator!=(const build_provenance& lhs, const build_provenance& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const build_provenance& lhs, const build_provenance& rhs) noexcept { return std::tie(lhs.candidate_control_, lhs.build_inputs_, lhs.build_result_, lhs.artifact_, lhs.artifact_manifest_) < std::tie(rhs.candidate_control_, rhs.build_inputs_, rhs.build_result_, rhs.artifact_, rhs.artifact_manifest_); }

installed_control installed_control::make(
    package_source_record source,
    installation_reason reason,
    build_provenance build)
{
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
