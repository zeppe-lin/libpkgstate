// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/installed_control.h>

#include "canonical_record.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include <libpkgstate/error.h>

namespace pkgstate {
namespace {

void
validate_line_safe(std::string_view value,
                   const char* label,
                   bool allow_empty = false)
{
  if (!allow_empty && value.empty())
    throw identity_error(std::string(label) + " must not be empty");

  for (const char byte : value)
  {
    if (byte == '\0' || byte == '\n' || byte == '\r')
      throw identity_error(std::string(label) + " is not line-safe");
  }
}

void
validate_fact_state(installed_control_fact_state state)
{
  switch (state)
  {
    case installed_control_fact_state::recorded_at_installation:
    case installed_control_fact_state::recorded_in_compatibility_storage:
    case installed_control_fact_state::supplied_by_migration:
    case installed_control_fact_state::historically_unavailable:
      return;
  }
  throw state_error("invalid installed-control fact state");
}

void
validate_lifecycle_phase(removal_lifecycle_phase phase)
{
  switch (phase)
  {
    case removal_lifecycle_phase::pre_remove:
    case removal_lifecycle_phase::post_remove:
      return;
  }
  throw state_error("invalid removal lifecycle phase");
}

void
validate_provenance_kind(control_provenance_kind kind)
{
  switch (kind)
  {
    case control_provenance_kind::candidate_control:
    case control_provenance_kind::artifact:
    case control_provenance_kind::artifact_manifest:
    case control_provenance_kind::application_evidence:
    case control_provenance_kind::transaction_evidence:
      return;
  }
  throw state_error("invalid installed-control provenance kind");
}

template<typename Values>
void
validate_availability(installed_control_fact_state state,
                      const Values& values,
                      const char* label)
{
  validate_fact_state(state);
  if (!is_known(state) && !values.empty())
  {
    throw state_error(std::string(label) +
                      " cannot contain values when historically unavailable");
  }
}

installed_control_identity
identify_control(const package_release& release,
                 const installed_control_completeness& completeness,
                 const std::vector<runtime_dependency_declaration>&
                     runtime_dependencies,
                 const std::vector<removal_lifecycle_declaration>&
                     removal_lifecycle,
                 const std::vector<target_profile_fact>& target_profile,
                 const std::vector<control_provenance>& provenance)
{
  detail::canonical_record record(
      installed_control_identity::canonical_domain());
  record.append_digest(release.identity());

  record.append_u8(static_cast<std::uint8_t>(
      completeness.runtime_dependencies));
  record.append_u64(static_cast<std::uint64_t>(runtime_dependencies.size()));
  for (const runtime_dependency_declaration& dependency : runtime_dependencies)
    record.append_bytes(dependency.expression());

  record.append_u8(static_cast<std::uint8_t>(
      completeness.removal_lifecycle));
  record.append_u64(static_cast<std::uint64_t>(removal_lifecycle.size()));
  for (const removal_lifecycle_declaration& declaration : removal_lifecycle)
  {
    record.append_u8(static_cast<std::uint8_t>(declaration.phase()));
    record.append_bytes(declaration.format());
    record.append_bytes(declaration.material());
  }

  record.append_u8(static_cast<std::uint8_t>(completeness.target_profile));
  record.append_u64(static_cast<std::uint64_t>(target_profile.size()));
  for (const target_profile_fact& fact : target_profile)
  {
    record.append_bytes(fact.name());
    record.append_bytes(fact.value());
  }

  record.append_u8(static_cast<std::uint8_t>(completeness.provenance));
  record.append_u64(static_cast<std::uint64_t>(provenance.size()));
  for (const control_provenance& reference : provenance)
  {
    record.append_u8(static_cast<std::uint8_t>(reference.kind()));
    record.append_bytes(reference.identity());
  }

  return installed_control_identity::from_sha256(record.sha256());
}

} // namespace

bool
is_known(installed_control_fact_state state) noexcept
{
  switch (state)
  {
    case installed_control_fact_state::recorded_at_installation:
    case installed_control_fact_state::recorded_in_compatibility_storage:
    case installed_control_fact_state::supplied_by_migration:
      return true;
    case installed_control_fact_state::historically_unavailable:
      return false;
  }
  return false;
}

runtime_dependency_declaration
runtime_dependency_declaration::make(std::string_view expression)
{
  validate_line_safe(expression, "runtime dependency declaration");
  return runtime_dependency_declaration(std::string(expression));
}

runtime_dependency_declaration::runtime_dependency_declaration(
    std::string expression)
    : expression_(std::move(expression))
{
}

const std::string&
runtime_dependency_declaration::expression() const noexcept
{
  return expression_;
}

bool
operator==(const runtime_dependency_declaration& lhs,
           const runtime_dependency_declaration& rhs) noexcept
{
  return lhs.expression_ == rhs.expression_;
}

bool
operator!=(const runtime_dependency_declaration& lhs,
           const runtime_dependency_declaration& rhs) noexcept
{
  return !(lhs == rhs);
}

bool
operator<(const runtime_dependency_declaration& lhs,
          const runtime_dependency_declaration& rhs) noexcept
{
  return lhs.expression_ < rhs.expression_;
}

removal_lifecycle_declaration
removal_lifecycle_declaration::make(removal_lifecycle_phase phase,
                                    std::string_view format,
                                    std::string material)
{
  validate_lifecycle_phase(phase);
  validate_line_safe(format, "removal lifecycle format");
  return removal_lifecycle_declaration(
      phase, std::string(format), std::move(material));
}

removal_lifecycle_declaration::removal_lifecycle_declaration(
    removal_lifecycle_phase phase,
    std::string format,
    std::string material)
    : phase_(phase), format_(std::move(format)), material_(std::move(material))
{
}

removal_lifecycle_phase
removal_lifecycle_declaration::phase() const noexcept
{
  return phase_;
}

const std::string&
removal_lifecycle_declaration::format() const noexcept
{
  return format_;
}

const std::string&
removal_lifecycle_declaration::material() const noexcept
{
  return material_;
}

bool
operator==(const removal_lifecycle_declaration& lhs,
           const removal_lifecycle_declaration& rhs) noexcept
{
  return lhs.phase_ == rhs.phase_ && lhs.format_ == rhs.format_ &&
         lhs.material_ == rhs.material_;
}

bool
operator!=(const removal_lifecycle_declaration& lhs,
           const removal_lifecycle_declaration& rhs) noexcept
{
  return !(lhs == rhs);
}

bool
operator<(const removal_lifecycle_declaration& lhs,
          const removal_lifecycle_declaration& rhs) noexcept
{
  return std::tie(lhs.phase_, lhs.format_, lhs.material_) <
         std::tie(rhs.phase_, rhs.format_, rhs.material_);
}

target_profile_fact
target_profile_fact::make(std::string_view name, std::string_view value)
{
  validate_line_safe(name, "target profile fact name");
  validate_line_safe(value, "target profile fact value", true);
  return target_profile_fact(std::string(name), std::string(value));
}

target_profile_fact::target_profile_fact(std::string name, std::string value)
    : name_(std::move(name)), value_(std::move(value))
{
}

const std::string&
target_profile_fact::name() const noexcept
{
  return name_;
}

const std::string&
target_profile_fact::value() const noexcept
{
  return value_;
}

bool
operator==(const target_profile_fact& lhs,
           const target_profile_fact& rhs) noexcept
{
  return lhs.name_ == rhs.name_ && lhs.value_ == rhs.value_;
}

bool
operator!=(const target_profile_fact& lhs,
           const target_profile_fact& rhs) noexcept
{
  return !(lhs == rhs);
}

bool
operator<(const target_profile_fact& lhs,
          const target_profile_fact& rhs) noexcept
{
  return std::tie(lhs.name_, lhs.value_) < std::tie(rhs.name_, rhs.value_);
}

control_provenance
control_provenance::make(control_provenance_kind kind,
                         std::string_view identity)
{
  validate_provenance_kind(kind);
  const detail::digest_value parsed = detail::digest_value::parse(identity);
  return control_provenance(kind, parsed.string());
}

control_provenance::control_provenance(control_provenance_kind kind,
                                       std::string identity)
    : kind_(kind), identity_(std::move(identity))
{
}

control_provenance_kind
control_provenance::kind() const noexcept
{
  return kind_;
}

const std::string&
control_provenance::identity() const noexcept
{
  return identity_;
}

bool
operator==(const control_provenance& lhs,
           const control_provenance& rhs) noexcept
{
  return lhs.kind_ == rhs.kind_ && lhs.identity_ == rhs.identity_;
}

bool
operator!=(const control_provenance& lhs,
           const control_provenance& rhs) noexcept
{
  return !(lhs == rhs);
}

bool
operator<(const control_provenance& lhs,
          const control_provenance& rhs) noexcept
{
  return std::tie(lhs.kind_, lhs.identity_) <
         std::tie(rhs.kind_, rhs.identity_);
}

bool
operator==(const installed_control_completeness& lhs,
           const installed_control_completeness& rhs) noexcept
{
  return lhs.runtime_dependencies == rhs.runtime_dependencies &&
         lhs.removal_lifecycle == rhs.removal_lifecycle &&
         lhs.target_profile == rhs.target_profile &&
         lhs.provenance == rhs.provenance;
}

bool
operator!=(const installed_control_completeness& lhs,
           const installed_control_completeness& rhs) noexcept
{
  return !(lhs == rhs);
}

bool
operator<(const installed_control_completeness& lhs,
          const installed_control_completeness& rhs) noexcept
{
  return std::tie(lhs.runtime_dependencies,
                  lhs.removal_lifecycle,
                  lhs.target_profile,
                  lhs.provenance) <
         std::tie(rhs.runtime_dependencies,
                  rhs.removal_lifecycle,
                  rhs.target_profile,
                  rhs.provenance);
}

installed_control
installed_control::make(
    package_release release,
    installed_control_completeness completeness,
    std::vector<runtime_dependency_declaration> runtime_dependencies,
    std::vector<removal_lifecycle_declaration> removal_lifecycle,
    std::vector<target_profile_fact> target_profile,
    std::vector<control_provenance> provenance)
{
  validate_availability(completeness.runtime_dependencies,
                        runtime_dependencies,
                        "runtime dependencies");
  validate_availability(completeness.removal_lifecycle,
                        removal_lifecycle,
                        "removal lifecycle");
  validate_availability(completeness.target_profile,
                        target_profile,
                        "target profile");
  validate_availability(completeness.provenance,
                        provenance,
                        "provenance");

  std::sort(runtime_dependencies.begin(), runtime_dependencies.end());
  const auto duplicate_dependency = std::adjacent_find(
      runtime_dependencies.begin(), runtime_dependencies.end());
  if (duplicate_dependency != runtime_dependencies.end())
  {
    throw state_error("duplicate runtime dependency declaration: " +
                      duplicate_dependency->expression());
  }

  std::stable_sort(
      removal_lifecycle.begin(), removal_lifecycle.end(),
      [](const removal_lifecycle_declaration& lhs,
         const removal_lifecycle_declaration& rhs) {
        return lhs.phase() < rhs.phase();
      });

  std::sort(target_profile.begin(), target_profile.end());
  const auto duplicate_profile = std::adjacent_find(
      target_profile.begin(), target_profile.end(),
      [](const target_profile_fact& lhs, const target_profile_fact& rhs) {
        return lhs.name() == rhs.name();
      });
  if (duplicate_profile != target_profile.end())
  {
    throw state_error("duplicate target profile fact: " +
                      duplicate_profile->name());
  }

  std::sort(provenance.begin(), provenance.end());
  const auto duplicate_provenance = std::adjacent_find(
      provenance.begin(), provenance.end(),
      [](const control_provenance& lhs, const control_provenance& rhs) {
        return lhs.kind() == rhs.kind();
      });
  if (duplicate_provenance != provenance.end())
    throw state_error("duplicate installed-control provenance kind");

  const installed_control_identity identity = identify_control(
      release,
      completeness,
      runtime_dependencies,
      removal_lifecycle,
      target_profile,
      provenance);

  return installed_control(std::move(identity),
                           std::move(release),
                           completeness,
                           std::move(runtime_dependencies),
                           std::move(removal_lifecycle),
                           std::move(target_profile),
                           std::move(provenance));
}

installed_control::installed_control(
    installed_control_identity identity,
    package_release release,
    installed_control_completeness completeness,
    std::vector<runtime_dependency_declaration> runtime_dependencies,
    std::vector<removal_lifecycle_declaration> removal_lifecycle,
    std::vector<target_profile_fact> target_profile,
    std::vector<control_provenance> provenance)
    : identity_(std::move(identity)),
      release_(std::move(release)),
      completeness_(completeness),
      runtime_dependencies_(std::move(runtime_dependencies)),
      removal_lifecycle_(std::move(removal_lifecycle)),
      target_profile_(std::move(target_profile)),
      provenance_(std::move(provenance))
{
}

const installed_control_identity&
installed_control::identity() const noexcept
{
  return identity_;
}

const package_release&
installed_control::release() const noexcept
{
  return release_;
}

const installed_control_completeness&
installed_control::completeness() const noexcept
{
  return completeness_;
}

const std::vector<runtime_dependency_declaration>&
installed_control::runtime_dependencies() const noexcept
{
  return runtime_dependencies_;
}

const std::vector<removal_lifecycle_declaration>&
installed_control::removal_lifecycle() const noexcept
{
  return removal_lifecycle_;
}

const std::vector<target_profile_fact>&
installed_control::target_profile() const noexcept
{
  return target_profile_;
}

const std::vector<control_provenance>&
installed_control::provenance() const noexcept
{
  return provenance_;
}

bool
operator==(const installed_control& lhs,
           const installed_control& rhs) noexcept
{
  return lhs.identity_ == rhs.identity_ && lhs.release_ == rhs.release_ &&
         lhs.completeness_ == rhs.completeness_ &&
         lhs.runtime_dependencies_ == rhs.runtime_dependencies_ &&
         lhs.removal_lifecycle_ == rhs.removal_lifecycle_ &&
         lhs.target_profile_ == rhs.target_profile_ &&
         lhs.provenance_ == rhs.provenance_;
}

bool
operator!=(const installed_control& lhs,
           const installed_control& rhs) noexcept
{
  return !(lhs == rhs);
}

bool
operator<(const installed_control& lhs,
          const installed_control& rhs) noexcept
{
  return std::tie(lhs.release_,
                  lhs.completeness_,
                  lhs.runtime_dependencies_,
                  lhs.removal_lifecycle_,
                  lhs.target_profile_,
                  lhs.provenance_,
                  lhs.identity_) <
         std::tie(rhs.release_,
                  rhs.completeness_,
                  rhs.runtime_dependencies_,
                  rhs.removal_lifecycle_,
                  rhs.target_profile_,
                  rhs.provenance_,
                  rhs.identity_);
}

} // namespace pkgstate
