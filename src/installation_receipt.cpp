// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/installation_receipt.h>

#include "canonical_record.h"

#include <algorithm>
#include <tuple>
#include <utility>

#include <libpkgstate/error.h>

namespace pkgstate {
namespace {

installation_receipt_identity identify(
    const installed_control& control,
    const state_target_binding& target_binding,
    const std::vector<owned_entry>& manifest,
    const operation_plan_identity& operation_plan,
    const application_evidence_identity& application_evidence,
    const std::optional<transaction_evidence_identity>& transaction_evidence)
{
  detail::canonical_record record(installation_receipt_identity::canonical_domain());
  record.append_u16(installation_receipt_schema_version);
  record.append_digest(control.identity());
  record.append_digest(target_binding.identity());
  record.append_u64(manifest.size());
  for (const owned_entry& entry : manifest)
  {
    record.append_bytes(entry.path().string());
    const installed_object_metadata& object = entry.object();
    record.append_u8(static_cast<std::uint8_t>(object.kind()));
    record.append_u32(object.mode());
    record.append_u64(object.uid());
    record.append_u64(object.gid());
    record.append_i64(object.mtime().seconds());
    record.append_u32(object.mtime().nanoseconds());
    record.append_bool(object.size().has_value());
    if (object.size())
      record.append_u64(*object.size());
    record.append_bool(object.regular_content().has_value());
    if (object.regular_content())
      record.append_digest(*object.regular_content());
    record.append_bool(object.symlink_target().has_value());
    if (object.symlink_target())
      record.append_bytes(*object.symlink_target());
    record.append_bool(object.device().has_value());
    if (object.device())
    {
      record.append_u64(object.device()->major());
      record.append_u64(object.device()->minor());
    }
    record.append_bool(object.hardlink_anchor().has_value());
    if (object.hardlink_anchor())
      record.append_bytes(object.hardlink_anchor()->string());
    record.append_u8(static_cast<std::uint8_t>(entry.origin()));
    record.append_bool(entry.rejected().has_value());
    if (entry.rejected())
    {
      record.append_u8(static_cast<std::uint8_t>(entry.rejected()->side()));
      record.append_digest(entry.rejected()->identity());
    }
  }
  record.append_digest(operation_plan);
  record.append_digest(application_evidence);
  record.append_bool(transaction_evidence.has_value());
  if (transaction_evidence)
    record.append_digest(*transaction_evidence);
  return installation_receipt_identity::from_sha256(record.sha256());
}

} // namespace

installation_receipt installation_receipt::make(
    installed_control control,
    state_target_binding target_binding,
    std::vector<owned_entry> manifest,
    operation_plan_identity operation_plan,
    application_evidence_identity application_evidence,
    std::optional<transaction_evidence_identity> transaction_evidence)
{
  std::sort(manifest.begin(), manifest.end(),
            [](const owned_entry& lhs, const owned_entry& rhs) {
              return lhs.path() < rhs.path();
            });
  for (std::size_t index = 1; index < manifest.size(); ++index)
    if (manifest[index - 1].path() == manifest[index].path())
      throw state_error("duplicate path in installation receipt manifest");

  installation_receipt_identity identity = identify(
      control, target_binding, manifest, operation_plan,
      application_evidence, transaction_evidence);
  return installation_receipt(
      std::move(identity), std::move(control), std::move(target_binding),
      std::move(manifest), std::move(operation_plan),
      std::move(application_evidence), std::move(transaction_evidence));
}

installation_receipt::installation_receipt(
    installation_receipt_identity identity,
    installed_control control,
    state_target_binding target_binding,
    std::vector<owned_entry> manifest,
    operation_plan_identity operation_plan,
    application_evidence_identity application_evidence,
    std::optional<transaction_evidence_identity> transaction_evidence)
    : identity_(std::move(identity)), control_(std::move(control)),
      target_binding_(std::move(target_binding)), manifest_(std::move(manifest)),
      operation_plan_(std::move(operation_plan)),
      application_evidence_(std::move(application_evidence)),
      transaction_evidence_(std::move(transaction_evidence))
{
}

std::uint16_t installation_receipt::schema_version() const noexcept { return installation_receipt_schema_version; }
const installation_receipt_identity& installation_receipt::identity() const noexcept { return identity_; }
const installed_control& installation_receipt::control() const noexcept { return control_; }
const package_release& installation_receipt::release() const noexcept { return control_.release(); }
const state_target_binding& installation_receipt::target_binding() const noexcept { return target_binding_; }
const std::vector<owned_entry>& installation_receipt::manifest() const noexcept { return manifest_; }
const operation_plan_identity& installation_receipt::operation_plan() const noexcept { return operation_plan_; }
const application_evidence_identity& installation_receipt::application_evidence() const noexcept { return application_evidence_; }
const std::optional<transaction_evidence_identity>& installation_receipt::transaction_evidence() const noexcept { return transaction_evidence_; }
bool operator==(const installation_receipt& lhs, const installation_receipt& rhs) noexcept { return lhs.identity_ == rhs.identity_ && std::tie(lhs.control_, lhs.target_binding_, lhs.manifest_, lhs.operation_plan_, lhs.application_evidence_, lhs.transaction_evidence_) == std::tie(rhs.control_, rhs.target_binding_, rhs.manifest_, rhs.operation_plan_, rhs.application_evidence_, rhs.transaction_evidence_); }
bool operator!=(const installation_receipt& lhs, const installation_receipt& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const installation_receipt& lhs, const installation_receipt& rhs) noexcept { return lhs.identity_ < rhs.identity_; }

} // namespace pkgstate
