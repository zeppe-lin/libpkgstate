// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/model.h>

#include <algorithm>
#include <tuple>
#include <utility>

#include <libpkgstate/error.h>

namespace pkgstate {
namespace {

bool canonical_atom(std::string_view value, bool allow_at)
{
  std::size_t offset = 0;
  if (allow_at)
  {
    if (value.empty() || value.front() != '@')
      return false;
    offset = 1;
  }
  if (offset == value.size())
    return false;
  if (value[offset] < 'a' || value[offset] > 'z')
    return false;
  for (std::size_t index = offset + 1; index < value.size(); ++index)
  {
    const char value_byte = value[index];
    if (!((value_byte >= 'a' && value_byte <= 'z') ||
          (value_byte >= '0' && value_byte <= '9') ||
          value_byte == '+' || value_byte == '.' || value_byte == '_' ||
          value_byte == '-'))
      return false;
  }
  return true;
}

bool line_safe(std::string_view value)
{
  if (value.empty())
    return false;
  for (const unsigned char byte : value)
    if (byte == 0 || byte == '\n' || byte == '\r' || byte < 0x20 || byte == 0x7f)
      return false;
  return true;
}

bool text_safe(std::string_view value)
{
  if (value.empty())
    return false;
  for (const unsigned char byte : value)
    if (byte == 0 || (byte < 0x20 && byte != '\n' && byte != '\t') || byte == 0x7f)
      return false;
  return true;
}

template<typename Value>
void sort_unique(std::vector<Value>& values, const char* label)
{
  std::sort(values.begin(), values.end());
  if (std::adjacent_find(values.begin(), values.end()) != values.end())
    throw state_error(std::string("duplicate normalized ") + label);
}

void validate_lifecycle(lifecycle_action action)
{
  switch (action)
  {
    case lifecycle_action::pre_install:
    case lifecycle_action::post_install:
    case lifecycle_action::pre_remove:
    case lifecycle_action::post_remove:
      return;
  }
  throw state_error("invalid lifecycle action");
}

void validate_program_language(program_language language)
{
  if (language != program_language::posix_shell)
    throw state_error("invalid program language");
}

void validate_member(requirement_member_kind kind, std::string_view member)
{
  switch (kind)
  {
    case requirement_member_kind::package:
      (void)package_reference(std::string(member));
      return;
    case requirement_member_kind::profile:
      (void)profile_reference(std::string(member));
      return;
  }
  throw state_error("invalid requirement member kind");
}

} // namespace

std::string_view to_string(lifecycle_action value) noexcept
{
  switch (value)
  {
    case lifecycle_action::pre_install: return "pre-install";
    case lifecycle_action::post_install: return "post-install";
    case lifecycle_action::pre_remove: return "pre-remove";
    case lifecycle_action::post_remove: return "post-remove";
  }
  return "unknown";
}

std::string_view to_string(program_language value) noexcept
{
  return value == program_language::posix_shell ? "posix-shell" : "unknown";
}

package_reference::package_reference(std::string name) : name_(std::move(name))
{
  if (!canonical_atom(name_, false))
    throw identity_error("invalid canonical package reference: " + name_);
}
const std::string& package_reference::name() const noexcept { return name_; }
bool operator==(const package_reference& lhs, const package_reference& rhs) noexcept { return lhs.name_ == rhs.name_; }
bool operator!=(const package_reference& lhs, const package_reference& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const package_reference& lhs, const package_reference& rhs) noexcept { return lhs.name_ < rhs.name_; }

profile_reference::profile_reference(std::string name) : name_(std::move(name))
{
  if (!canonical_atom(name_, true))
    throw identity_error("invalid canonical profile reference: " + name_);
}
const std::string& profile_reference::name() const noexcept { return name_; }
bool operator==(const profile_reference& lhs, const profile_reference& rhs) noexcept { return lhs.name_ == rhs.name_; }
bool operator!=(const profile_reference& lhs, const profile_reference& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const profile_reference& lhs, const profile_reference& rhs) noexcept { return lhs.name_ < rhs.name_; }

architecture_reference::architecture_reference(std::string name) : name_(std::move(name))
{
  if (!canonical_atom(name_, false))
    throw identity_error("invalid canonical architecture reference: " + name_);
}
const std::string& architecture_reference::name() const noexcept { return name_; }
bool operator==(const architecture_reference& lhs, const architecture_reference& rhs) noexcept { return lhs.name_ == rhs.name_; }
bool operator!=(const architecture_reference& lhs, const architecture_reference& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const architecture_reference& lhs, const architecture_reference& rhs) noexcept { return lhs.name_ < rhs.name_; }

declaration_provenance::declaration_provenance(
    std::string document, std::string path, std::uint32_t line,
    std::uint32_t column)
    : document_(std::move(document)), path_(std::move(path)),
      line_(line), column_(column)
{
  if (!line_safe(document_) || !line_safe(path_) || line_ == 0 || column_ == 0)
    throw state_error("invalid declaration provenance");
}
const std::string& declaration_provenance::document() const noexcept { return document_; }
const std::string& declaration_provenance::path() const noexcept { return path_; }
std::uint32_t declaration_provenance::line() const noexcept { return line_; }
std::uint32_t declaration_provenance::column() const noexcept { return column_; }
bool operator==(const declaration_provenance& lhs, const declaration_provenance& rhs) noexcept { return std::tie(lhs.document_, lhs.path_, lhs.line_, lhs.column_) == std::tie(rhs.document_, rhs.path_, rhs.line_, rhs.column_); }
bool operator!=(const declaration_provenance& lhs, const declaration_provenance& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const declaration_provenance& lhs, const declaration_provenance& rhs) noexcept { return std::tie(lhs.document_, lhs.path_, lhs.line_, lhs.column_) < std::tie(rhs.document_, rhs.path_, rhs.line_, rhs.column_); }

profile_expansion_step::profile_expansion_step(
    profile_reference profile, requirement_member_kind member_kind,
    std::string member, declaration_provenance provenance)
    : profile_(std::move(profile)), member_kind_(member_kind),
      member_(std::move(member)), provenance_(std::move(provenance))
{
  validate_member(member_kind_, member_);
}
const profile_reference& profile_expansion_step::profile() const noexcept { return profile_; }
requirement_member_kind profile_expansion_step::member_kind() const noexcept { return member_kind_; }
const std::string& profile_expansion_step::member() const noexcept { return member_; }
const declaration_provenance& profile_expansion_step::provenance() const noexcept { return provenance_; }
bool operator==(const profile_expansion_step& lhs, const profile_expansion_step& rhs) noexcept { return std::tie(lhs.profile_, lhs.member_kind_, lhs.member_, lhs.provenance_) == std::tie(rhs.profile_, rhs.member_kind_, rhs.member_, rhs.provenance_); }
bool operator!=(const profile_expansion_step& lhs, const profile_expansion_step& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const profile_expansion_step& lhs, const profile_expansion_step& rhs) noexcept { return std::tie(lhs.profile_, lhs.member_kind_, lhs.member_, lhs.provenance_) < std::tie(rhs.profile_, rhs.member_kind_, rhs.member_, rhs.provenance_); }

requirement_origin::requirement_origin(
    declaration_provenance declaration,
    std::vector<profile_expansion_step> expansion)
    : declaration_(std::move(declaration)), expansion_(std::move(expansion))
{
}
const declaration_provenance& requirement_origin::declaration() const noexcept { return declaration_; }
const std::vector<profile_expansion_step>& requirement_origin::expansion() const noexcept { return expansion_; }
bool operator==(const requirement_origin& lhs, const requirement_origin& rhs) noexcept { return std::tie(lhs.declaration_, lhs.expansion_) == std::tie(rhs.declaration_, rhs.expansion_); }
bool operator!=(const requirement_origin& lhs, const requirement_origin& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const requirement_origin& lhs, const requirement_origin& rhs) noexcept { return std::tie(lhs.declaration_, lhs.expansion_) < std::tie(rhs.declaration_, rhs.expansion_); }

package_requirement::package_requirement(
    package_reference package, std::vector<requirement_origin> origins)
    : package_(std::move(package)), origins_(std::move(origins))
{
  if (origins_.empty())
    throw state_error("package requirement has no retained origin");
  sort_unique(origins_, "requirement origin");
  for (const requirement_origin& origin : origins_)
  {
    if (!origin.expansion().empty())
    {
      const profile_expansion_step& leaf = origin.expansion().back();
      if (leaf.member_kind() != requirement_member_kind::package ||
          leaf.member() != package_.name())
        throw state_error("profile expansion does not terminate at requirement package");
    }
  }
}
const package_reference& package_requirement::package() const noexcept { return package_; }
const std::vector<requirement_origin>& package_requirement::origins() const noexcept { return origins_; }
bool operator==(const package_requirement& lhs, const package_requirement& rhs) noexcept { return std::tie(lhs.package_, lhs.origins_) == std::tie(rhs.package_, rhs.origins_); }
bool operator!=(const package_requirement& lhs, const package_requirement& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const package_requirement& lhs, const package_requirement& rhs) noexcept { return std::tie(lhs.package_, lhs.origins_) < std::tie(rhs.package_, rhs.origins_); }

program::program(program_language language, std::string material)
    : language_(language), material_(std::move(material))
{
  validate_program_language(language_);
  if (!text_safe(material_))
    throw state_error("invalid empty or binary program");
}
program_language program::language() const noexcept { return language_; }
const std::string& program::material() const noexcept { return material_; }
bool operator==(const program& lhs, const program& rhs) noexcept { return std::tie(lhs.language_, lhs.material_) == std::tie(rhs.language_, rhs.material_); }
bool operator!=(const program& lhs, const program& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const program& lhs, const program& rhs) noexcept { return std::tie(lhs.language_, lhs.material_) < std::tie(rhs.language_, rhs.material_); }

lifecycle_program::lifecycle_program(lifecycle_action action, program value)
    : action_(action), value_(std::move(value))
{
  validate_lifecycle(action_);
}
lifecycle_action lifecycle_program::action() const noexcept { return action_; }
const program& lifecycle_program::value() const noexcept { return value_; }
bool operator==(const lifecycle_program& lhs, const lifecycle_program& rhs) noexcept { return std::tie(lhs.action_, lhs.value_) == std::tie(rhs.action_, rhs.value_); }
bool operator!=(const lifecycle_program& lhs, const lifecycle_program& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const lifecycle_program& lhs, const lifecycle_program& rhs) noexcept { return std::tie(lhs.action_, lhs.value_) < std::tie(rhs.action_, rhs.value_); }

lifecycle_requirement::lifecycle_requirement(
    lifecycle_action action, package_requirement requirement)
    : action_(action), requirement_(std::move(requirement))
{
  validate_lifecycle(action_);
}
lifecycle_action lifecycle_requirement::action() const noexcept { return action_; }
const package_requirement& lifecycle_requirement::requirement() const noexcept { return requirement_; }
bool operator==(const lifecycle_requirement& lhs, const lifecycle_requirement& rhs) noexcept { return std::tie(lhs.action_, lhs.requirement_) == std::tie(rhs.action_, rhs.requirement_); }
bool operator!=(const lifecycle_requirement& lhs, const lifecycle_requirement& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const lifecycle_requirement& lhs, const lifecycle_requirement& rhs) noexcept { return std::tie(lhs.action_, lhs.requirement_) < std::tie(rhs.action_, rhs.requirement_); }

package_metadata::package_metadata(
    std::string summary, std::optional<std::string> description,
    std::optional<std::string> homepage, std::vector<std::string> licenses)
    : summary_(std::move(summary)), description_(std::move(description)),
      homepage_(std::move(homepage)), licenses_(std::move(licenses))
{
  if (!line_safe(summary_) || (description_ && !text_safe(*description_)) ||
      (homepage_ && !line_safe(*homepage_)) || licenses_.empty())
    throw state_error("invalid package metadata");
  for (const std::string& license : licenses_)
    if (!line_safe(license))
      throw state_error("invalid package license");
  sort_unique(licenses_, "package license");
}
const std::string& package_metadata::summary() const noexcept { return summary_; }
const std::optional<std::string>& package_metadata::description() const noexcept { return description_; }
const std::optional<std::string>& package_metadata::homepage() const noexcept { return homepage_; }
const std::vector<std::string>& package_metadata::licenses() const noexcept { return licenses_; }
bool operator==(const package_metadata& lhs, const package_metadata& rhs) noexcept { return std::tie(lhs.summary_, lhs.description_, lhs.homepage_, lhs.licenses_) == std::tie(rhs.summary_, rhs.description_, rhs.homepage_, rhs.licenses_); }
bool operator!=(const package_metadata& lhs, const package_metadata& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const package_metadata& lhs, const package_metadata& rhs) noexcept { return std::tie(lhs.summary_, lhs.description_, lhs.homepage_, lhs.licenses_) < std::tie(rhs.summary_, rhs.description_, rhs.homepage_, rhs.licenses_); }

selected_profile::selected_profile(
    profile_reference profile, source_profile_identity identity,
    std::vector<declaration_provenance> declarations)
    : profile_(std::move(profile)), identity_(std::move(identity)),
      declarations_(std::move(declarations))
{
  if (declarations_.empty())
    throw state_error("selected profile has no issuing declaration");
  sort_unique(declarations_, "selected-profile declaration");
}
const profile_reference& selected_profile::profile() const noexcept { return profile_; }
const source_profile_identity& selected_profile::identity() const noexcept { return identity_; }
const std::vector<declaration_provenance>& selected_profile::declarations() const noexcept { return declarations_; }
bool operator==(const selected_profile& lhs, const selected_profile& rhs) noexcept { return std::tie(lhs.profile_, lhs.identity_, lhs.declarations_) == std::tie(rhs.profile_, rhs.identity_, rhs.declarations_); }
bool operator!=(const selected_profile& lhs, const selected_profile& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const selected_profile& lhs, const selected_profile& rhs) noexcept { return std::tie(lhs.profile_, lhs.identity_, lhs.declarations_) < std::tie(rhs.profile_, rhs.identity_, rhs.declarations_); }

architecture_binding architecture_binding::make(
    std::vector<architecture_reference> declared_build,
    std::vector<architecture_reference> declared_target,
    architecture_reference selected_build,
    architecture_reference selected_target)
{
  sort_unique(declared_build, "declared build architecture");
  sort_unique(declared_target, "declared target architecture");
  if (!declared_build.empty() &&
      !std::binary_search(declared_build.begin(), declared_build.end(), selected_build))
    throw state_error("selected build architecture is not permitted by source");
  if (!declared_target.empty() &&
      !std::binary_search(declared_target.begin(), declared_target.end(), selected_target))
    throw state_error("selected target architecture is not permitted by source");
  return architecture_binding(std::move(declared_build), std::move(declared_target),
                              std::move(selected_build), std::move(selected_target));
}
architecture_binding::architecture_binding(
    std::vector<architecture_reference> declared_build,
    std::vector<architecture_reference> declared_target,
    architecture_reference selected_build,
    architecture_reference selected_target)
    : declared_build_(std::move(declared_build)),
      declared_target_(std::move(declared_target)),
      selected_build_(std::move(selected_build)),
      selected_target_(std::move(selected_target))
{
}
const std::vector<architecture_reference>& architecture_binding::declared_build() const noexcept { return declared_build_; }
const std::vector<architecture_reference>& architecture_binding::declared_target() const noexcept { return declared_target_; }
const architecture_reference& architecture_binding::selected_build() const noexcept { return selected_build_; }
const architecture_reference& architecture_binding::selected_target() const noexcept { return selected_target_; }
bool operator==(const architecture_binding& lhs, const architecture_binding& rhs) noexcept { return std::tie(lhs.declared_build_, lhs.declared_target_, lhs.selected_build_, lhs.selected_target_) == std::tie(rhs.declared_build_, rhs.declared_target_, rhs.selected_build_, rhs.selected_target_); }
bool operator!=(const architecture_binding& lhs, const architecture_binding& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const architecture_binding& lhs, const architecture_binding& rhs) noexcept { return std::tie(lhs.declared_build_, lhs.declared_target_, lhs.selected_build_, lhs.selected_target_) < std::tie(rhs.declared_build_, rhs.declared_target_, rhs.selected_build_, rhs.selected_target_); }

} // namespace pkgstate
