// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/package_source_record.h>

#include "canonical_record.h"

#include <algorithm>
#include <tuple>
#include <utility>

#include <libpkgstate/error.h>

namespace pkgstate {
namespace {

void append_provenance(detail::canonical_record& record,
                       const declaration_provenance& provenance)
{
  record.append_bytes(provenance.document());
  record.append_bytes(provenance.path());
  record.append_u32(provenance.line());
  record.append_u32(provenance.column());
}

void append_requirement(detail::canonical_record& record,
                        const package_requirement& requirement)
{
  record.append_bytes(requirement.package().name());
  record.append_u64(requirement.origins().size());
  for (const requirement_origin& origin : requirement.origins())
  {
    append_provenance(record, origin.declaration());
    record.append_u64(origin.expansion().size());
    for (const profile_expansion_step& step : origin.expansion())
    {
      record.append_bytes(step.profile().name());
      record.append_u8(static_cast<std::uint8_t>(step.member_kind()));
      record.append_bytes(step.member());
      append_provenance(record, step.provenance());
    }
  }
}

package_source_record_identity identify(
    const package_release& release,
    const package_metadata& metadata,
    const std::vector<package_requirement>& runtime_requirements,
    const std::vector<lifecycle_program>& lifecycle_programs,
    const std::vector<lifecycle_requirement>& lifecycle_requirements,
    const architecture_binding& architectures,
    const std::vector<selected_profile>& selected_profiles,
    const source_snapshot_identity& snapshot)
{
  detail::canonical_record record(package_source_record_identity::canonical_domain());
  record.append_digest(release.identity());
  record.append_bytes(release.name());
  record.append_bytes(release.version());
  record.append_u32(release.release());

  record.append_bytes(metadata.summary());
  record.append_bool(metadata.description().has_value());
  if (metadata.description())
    record.append_bytes(*metadata.description());
  record.append_bool(metadata.homepage().has_value());
  if (metadata.homepage())
    record.append_bytes(*metadata.homepage());
  record.append_u64(metadata.licenses().size());
  for (const std::string& license : metadata.licenses())
    record.append_bytes(license);

  record.append_u64(runtime_requirements.size());
  for (const package_requirement& requirement : runtime_requirements)
    append_requirement(record, requirement);

  record.append_u64(lifecycle_programs.size());
  for (const lifecycle_program& lifecycle : lifecycle_programs)
  {
    record.append_u8(static_cast<std::uint8_t>(lifecycle.action()));
    record.append_u8(static_cast<std::uint8_t>(lifecycle.value().language()));
    record.append_bytes(lifecycle.value().material());
  }

  record.append_u64(lifecycle_requirements.size());
  for (const lifecycle_requirement& requirement : lifecycle_requirements)
  {
    record.append_u8(static_cast<std::uint8_t>(requirement.action()));
    append_requirement(record, requirement.requirement());
  }

  record.append_u64(architectures.declared_build().size());
  for (const architecture_reference& architecture : architectures.declared_build())
    record.append_bytes(architecture.name());
  record.append_u64(architectures.declared_target().size());
  for (const architecture_reference& architecture : architectures.declared_target())
    record.append_bytes(architecture.name());
  record.append_bytes(architectures.selected_build().name());
  record.append_bytes(architectures.selected_target().name());

  record.append_u64(selected_profiles.size());
  for (const selected_profile& profile : selected_profiles)
  {
    record.append_bytes(profile.profile().name());
    record.append_digest(profile.identity());
    record.append_u64(profile.declarations().size());
    for (const declaration_provenance& provenance : profile.declarations())
      append_provenance(record, provenance);
  }

  record.append_digest(snapshot);
  return package_source_record_identity::from_sha256(record.sha256());
}

template<typename Values, typename Key>
void reject_duplicate_keys(const Values& values, Key key, const char* label)
{
  for (std::size_t index = 1; index < values.size(); ++index)
    if (key(values[index - 1]) == key(values[index]))
      throw state_error(std::string("duplicate ") + label);
}

} // namespace

package_source_record package_source_record::make(
    package_release release,
    package_metadata metadata,
    std::vector<package_requirement> runtime_requirements,
    std::vector<lifecycle_program> lifecycle_programs,
    std::vector<lifecycle_requirement> lifecycle_requirements,
    architecture_binding architectures,
    std::vector<selected_profile> selected_profiles,
    source_snapshot_identity snapshot)
{
  std::sort(runtime_requirements.begin(), runtime_requirements.end(),
            [](const package_requirement& lhs, const package_requirement& rhs) {
              return lhs.package() < rhs.package();
            });
  reject_duplicate_keys(runtime_requirements,
                        [](const package_requirement& value) { return value.package().name(); },
                        "runtime requirement package");

  std::sort(lifecycle_programs.begin(), lifecycle_programs.end());
  reject_duplicate_keys(lifecycle_programs,
                        [](const lifecycle_program& value) { return value.action(); },
                        "lifecycle program action");

  std::sort(lifecycle_requirements.begin(), lifecycle_requirements.end(),
            [](const lifecycle_requirement& lhs, const lifecycle_requirement& rhs) {
              return std::make_tuple(lhs.action(), lhs.requirement().package()) <
                     std::make_tuple(rhs.action(), rhs.requirement().package());
            });
  for (std::size_t index = 1; index < lifecycle_requirements.size(); ++index)
  {
    const lifecycle_requirement& previous = lifecycle_requirements[index - 1];
    const lifecycle_requirement& current = lifecycle_requirements[index];
    if (previous.action() == current.action() &&
        previous.requirement().package() == current.requirement().package())
      throw state_error("duplicate lifecycle requirement package for action");
  }

  std::sort(selected_profiles.begin(), selected_profiles.end(),
            [](const selected_profile& lhs, const selected_profile& rhs) {
              return lhs.profile() < rhs.profile();
            });
  reject_duplicate_keys(selected_profiles,
                        [](const selected_profile& value) { return value.profile().name(); },
                        "selected profile");

  package_source_record_identity identity = identify(
      release, metadata, runtime_requirements, lifecycle_programs,
      lifecycle_requirements, architectures, selected_profiles, snapshot);
  return package_source_record(
      std::move(identity), std::move(release), std::move(metadata),
      std::move(runtime_requirements), std::move(lifecycle_programs),
      std::move(lifecycle_requirements), std::move(architectures),
      std::move(selected_profiles), std::move(snapshot));
}

package_source_record::package_source_record(
    package_source_record_identity identity,
    package_release release,
    package_metadata metadata,
    std::vector<package_requirement> runtime_requirements,
    std::vector<lifecycle_program> lifecycle_programs,
    std::vector<lifecycle_requirement> lifecycle_requirements,
    architecture_binding architectures,
    std::vector<selected_profile> selected_profiles,
    source_snapshot_identity snapshot)
    : identity_(std::move(identity)), release_(std::move(release)),
      metadata_(std::move(metadata)),
      runtime_requirements_(std::move(runtime_requirements)),
      lifecycle_programs_(std::move(lifecycle_programs)),
      lifecycle_requirements_(std::move(lifecycle_requirements)),
      architectures_(std::move(architectures)),
      selected_profiles_(std::move(selected_profiles)),
      snapshot_(std::move(snapshot))
{
}

const package_source_record_identity& package_source_record::identity() const noexcept { return identity_; }
const package_release& package_source_record::release() const noexcept { return release_; }
const package_metadata& package_source_record::metadata() const noexcept { return metadata_; }
const std::vector<package_requirement>& package_source_record::runtime_requirements() const noexcept { return runtime_requirements_; }
const std::vector<lifecycle_program>& package_source_record::lifecycle_programs() const noexcept { return lifecycle_programs_; }
const lifecycle_program* package_source_record::lifecycle(lifecycle_action action) const noexcept
{
  const auto found = std::lower_bound(lifecycle_programs_.begin(), lifecycle_programs_.end(), action,
      [](const lifecycle_program& value, lifecycle_action wanted) { return value.action() < wanted; });
  return found != lifecycle_programs_.end() && found->action() == action ? &*found : nullptr;
}
const std::vector<lifecycle_requirement>& package_source_record::lifecycle_requirements() const noexcept { return lifecycle_requirements_; }
std::vector<package_requirement> package_source_record::lifecycle_requirements(lifecycle_action action) const
{
  std::vector<package_requirement> result;
  for (const lifecycle_requirement& requirement : lifecycle_requirements_)
    if (requirement.action() == action)
      result.push_back(requirement.requirement());
  return result;
}
const architecture_binding& package_source_record::architectures() const noexcept { return architectures_; }
const std::vector<selected_profile>& package_source_record::selected_profiles() const noexcept { return selected_profiles_; }
const source_snapshot_identity& package_source_record::snapshot() const noexcept { return snapshot_; }
bool operator==(const package_source_record& lhs, const package_source_record& rhs) noexcept { return lhs.identity_ == rhs.identity_ && std::tie(lhs.release_, lhs.metadata_, lhs.runtime_requirements_, lhs.lifecycle_programs_, lhs.lifecycle_requirements_, lhs.architectures_, lhs.selected_profiles_, lhs.snapshot_) == std::tie(rhs.release_, rhs.metadata_, rhs.runtime_requirements_, rhs.lifecycle_programs_, rhs.lifecycle_requirements_, rhs.architectures_, rhs.selected_profiles_, rhs.snapshot_); }
bool operator!=(const package_source_record& lhs, const package_source_record& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const package_source_record& lhs, const package_source_record& rhs) noexcept { return lhs.identity_ < rhs.identity_; }

} // namespace pkgstate
