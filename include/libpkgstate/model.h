// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file model.h
 *  \brief Native installed-state value vocabulary.
 */
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <libpkgstate/digest.h>

namespace pkgstate {

enum class lifecycle_action : std::uint8_t {
  pre_install = 1,
  post_install = 2,
  pre_remove = 3,
  post_remove = 4,
};

enum class program_language : std::uint8_t { posix_shell = 1 };
enum class requirement_member_kind : std::uint8_t { package = 1, profile = 2 };

[[nodiscard]] std::string_view to_string(lifecycle_action value) noexcept;
[[nodiscard]] std::string_view to_string(program_language value) noexcept;

class package_reference final {
public:
  explicit package_reference(std::string name);
  [[nodiscard]] const std::string& name() const noexcept;
  friend bool operator==(const package_reference& lhs,
                         const package_reference& rhs) noexcept;
  friend bool operator!=(const package_reference& lhs,
                         const package_reference& rhs) noexcept;
  friend bool operator<(const package_reference& lhs,
                        const package_reference& rhs) noexcept;
private:
  std::string name_;
};

class profile_reference final {
public:
  explicit profile_reference(std::string name);
  [[nodiscard]] const std::string& name() const noexcept;
  friend bool operator==(const profile_reference& lhs,
                         const profile_reference& rhs) noexcept;
  friend bool operator!=(const profile_reference& lhs,
                         const profile_reference& rhs) noexcept;
  friend bool operator<(const profile_reference& lhs,
                        const profile_reference& rhs) noexcept;
private:
  std::string name_;
};

class architecture_reference final {
public:
  explicit architecture_reference(std::string name);
  [[nodiscard]] const std::string& name() const noexcept;
  friend bool operator==(const architecture_reference& lhs,
                         const architecture_reference& rhs) noexcept;
  friend bool operator!=(const architecture_reference& lhs,
                         const architecture_reference& rhs) noexcept;
  friend bool operator<(const architecture_reference& lhs,
                        const architecture_reference& rhs) noexcept;
private:
  std::string name_;
};

class declaration_provenance final {
public:
  declaration_provenance(std::string document, std::string path,
                         std::uint32_t line, std::uint32_t column);
  [[nodiscard]] const std::string& document() const noexcept;
  [[nodiscard]] const std::string& path() const noexcept;
  [[nodiscard]] std::uint32_t line() const noexcept;
  [[nodiscard]] std::uint32_t column() const noexcept;
  friend bool operator==(const declaration_provenance& lhs,
                         const declaration_provenance& rhs) noexcept;
  friend bool operator!=(const declaration_provenance& lhs,
                         const declaration_provenance& rhs) noexcept;
  friend bool operator<(const declaration_provenance& lhs,
                        const declaration_provenance& rhs) noexcept;
private:
  std::string document_;
  std::string path_;
  std::uint32_t line_;
  std::uint32_t column_;
};

class profile_expansion_step final {
public:
  profile_expansion_step(profile_reference profile,
                         requirement_member_kind member_kind,
                         std::string member,
                         declaration_provenance provenance);
  [[nodiscard]] const profile_reference& profile() const noexcept;
  [[nodiscard]] requirement_member_kind member_kind() const noexcept;
  [[nodiscard]] const std::string& member() const noexcept;
  [[nodiscard]] const declaration_provenance& provenance() const noexcept;
  friend bool operator==(const profile_expansion_step& lhs,
                         const profile_expansion_step& rhs) noexcept;
  friend bool operator!=(const profile_expansion_step& lhs,
                         const profile_expansion_step& rhs) noexcept;
  friend bool operator<(const profile_expansion_step& lhs,
                        const profile_expansion_step& rhs) noexcept;
private:
  profile_reference profile_;
  requirement_member_kind member_kind_;
  std::string member_;
  declaration_provenance provenance_;
};

class requirement_origin final {
public:
  requirement_origin(declaration_provenance declaration,
                     std::vector<profile_expansion_step> expansion = {});
  [[nodiscard]] const declaration_provenance& declaration() const noexcept;
  [[nodiscard]] const std::vector<profile_expansion_step>& expansion() const noexcept;
  friend bool operator==(const requirement_origin& lhs,
                         const requirement_origin& rhs) noexcept;
  friend bool operator!=(const requirement_origin& lhs,
                         const requirement_origin& rhs) noexcept;
  friend bool operator<(const requirement_origin& lhs,
                        const requirement_origin& rhs) noexcept;
private:
  declaration_provenance declaration_;
  std::vector<profile_expansion_step> expansion_;
};

class package_requirement final {
public:
  package_requirement(package_reference package,
                      std::vector<requirement_origin> origins);
  [[nodiscard]] const package_reference& package() const noexcept;
  [[nodiscard]] const std::vector<requirement_origin>& origins() const noexcept;
  friend bool operator==(const package_requirement& lhs,
                         const package_requirement& rhs) noexcept;
  friend bool operator!=(const package_requirement& lhs,
                         const package_requirement& rhs) noexcept;
  friend bool operator<(const package_requirement& lhs,
                        const package_requirement& rhs) noexcept;
private:
  package_reference package_;
  std::vector<requirement_origin> origins_;
};

class program final {
public:
  program(program_language language, std::string material);
  [[nodiscard]] program_language language() const noexcept;
  [[nodiscard]] const std::string& material() const noexcept;
  friend bool operator==(const program& lhs, const program& rhs) noexcept;
  friend bool operator!=(const program& lhs, const program& rhs) noexcept;
  friend bool operator<(const program& lhs, const program& rhs) noexcept;
private:
  program_language language_;
  std::string material_;
};

class lifecycle_program final {
public:
  lifecycle_program(lifecycle_action action, program value);
  [[nodiscard]] lifecycle_action action() const noexcept;
  [[nodiscard]] const program& value() const noexcept;
  friend bool operator==(const lifecycle_program& lhs,
                         const lifecycle_program& rhs) noexcept;
  friend bool operator!=(const lifecycle_program& lhs,
                         const lifecycle_program& rhs) noexcept;
  friend bool operator<(const lifecycle_program& lhs,
                        const lifecycle_program& rhs) noexcept;
private:
  lifecycle_action action_;
  program value_;
};

class lifecycle_requirement final {
public:
  lifecycle_requirement(lifecycle_action action, package_requirement requirement);
  [[nodiscard]] lifecycle_action action() const noexcept;
  [[nodiscard]] const package_requirement& requirement() const noexcept;
  friend bool operator==(const lifecycle_requirement& lhs,
                         const lifecycle_requirement& rhs) noexcept;
  friend bool operator!=(const lifecycle_requirement& lhs,
                         const lifecycle_requirement& rhs) noexcept;
  friend bool operator<(const lifecycle_requirement& lhs,
                        const lifecycle_requirement& rhs) noexcept;
private:
  lifecycle_action action_;
  package_requirement requirement_;
};

class package_metadata final {
public:
  package_metadata(std::string summary,
                   std::optional<std::string> description,
                   std::optional<std::string> homepage,
                   std::vector<std::string> licenses);
  [[nodiscard]] const std::string& summary() const noexcept;
  [[nodiscard]] const std::optional<std::string>& description() const noexcept;
  [[nodiscard]] const std::optional<std::string>& homepage() const noexcept;
  [[nodiscard]] const std::vector<std::string>& licenses() const noexcept;
  friend bool operator==(const package_metadata& lhs,
                         const package_metadata& rhs) noexcept;
  friend bool operator!=(const package_metadata& lhs,
                         const package_metadata& rhs) noexcept;
  friend bool operator<(const package_metadata& lhs,
                        const package_metadata& rhs) noexcept;
private:
  std::string summary_;
  std::optional<std::string> description_;
  std::optional<std::string> homepage_;
  std::vector<std::string> licenses_;
};

class selected_profile final {
public:
  selected_profile(profile_reference profile,
                   source_profile_identity identity,
                   std::vector<declaration_provenance> declarations);
  [[nodiscard]] const profile_reference& profile() const noexcept;
  [[nodiscard]] const source_profile_identity& identity() const noexcept;
  [[nodiscard]] const std::vector<declaration_provenance>& declarations() const noexcept;
  friend bool operator==(const selected_profile& lhs,
                         const selected_profile& rhs) noexcept;
  friend bool operator!=(const selected_profile& lhs,
                         const selected_profile& rhs) noexcept;
  friend bool operator<(const selected_profile& lhs,
                        const selected_profile& rhs) noexcept;
private:
  profile_reference profile_;
  source_profile_identity identity_;
  std::vector<declaration_provenance> declarations_;
};

class architecture_binding final {
public:
  [[nodiscard]] static architecture_binding make(
      std::vector<architecture_reference> declared_build,
      std::vector<architecture_reference> declared_target,
      architecture_reference selected_build,
      architecture_reference selected_target);
  [[nodiscard]] const std::vector<architecture_reference>& declared_build() const noexcept;
  [[nodiscard]] const std::vector<architecture_reference>& declared_target() const noexcept;
  [[nodiscard]] const architecture_reference& selected_build() const noexcept;
  [[nodiscard]] const architecture_reference& selected_target() const noexcept;
  friend bool operator==(const architecture_binding& lhs,
                         const architecture_binding& rhs) noexcept;
  friend bool operator!=(const architecture_binding& lhs,
                         const architecture_binding& rhs) noexcept;
  friend bool operator<(const architecture_binding& lhs,
                        const architecture_binding& rhs) noexcept;
private:
  architecture_binding(std::vector<architecture_reference> declared_build,
                       std::vector<architecture_reference> declared_target,
                       architecture_reference selected_build,
                       architecture_reference selected_target);
  std::vector<architecture_reference> declared_build_;
  std::vector<architecture_reference> declared_target_;
  architecture_reference selected_build_;
  architecture_reference selected_target_;
};

} // namespace pkgstate
