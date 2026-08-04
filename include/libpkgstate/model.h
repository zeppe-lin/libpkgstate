// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file model.h
 * \brief Durable package-source vocabulary retained by installed state.
 */
#pragma once

#include <libpkgstate/export.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <libpkgstate/digest.h>

namespace pkgstate {

/*! \brief Exact package lifecycle actions retained by installed control. */
enum class lifecycle_action : std::uint8_t {
  pre_install = 1,  //!< Run immediately before package installation.
  post_install = 2, //!< Run immediately after package installation.
  pre_remove = 3,   //!< Run immediately before package removal.
  post_remove = 4,  //!< Run immediately after package removal.
};

/*! \brief Languages retained for exact lifecycle program material. */
enum class program_language : std::uint8_t {
  posix_shell = 1, //!< Exact POSIX shell program bytes.
};

/*! \brief Kinds of members traversed during profile expansion. */
enum class requirement_member_kind : std::uint8_t {
  package = 1, //!< Expansion step names an exact package.
  profile = 2, //!< Expansion step names another profile.
};

/*! \brief Return canonical protocol spelling for a lifecycle action. */
[[nodiscard]] PKGSTATE_API std::string_view
to_string(lifecycle_action value) noexcept;

/*! \brief Return canonical protocol spelling for a program language. */
[[nodiscard]] PKGSTATE_API std::string_view
to_string(program_language value) noexcept;

/*! \brief Canonical exact package name retained by installed state. */
class PKGSTATE_API package_reference final {
public:
  /*!
   * \brief Construct a validated package reference.
   * \param name Lowercase package atom beginning with an ASCII letter.
   * \throws identity_error when \p name is not canonical.
   */
  explicit package_reference(std::string name);

  /*! \brief Return the canonical package name. */
  [[nodiscard]] const std::string& name() const noexcept;

  /*! \brief Compare package references for equality. */
  friend PKGSTATE_API bool operator==(const package_reference& lhs,
                                      const package_reference& rhs) noexcept;
  /*! \brief Compare package references for inequality. */
  friend PKGSTATE_API bool operator!=(const package_reference& lhs,
                                      const package_reference& rhs) noexcept;
  /*! \brief Order package references lexicographically. */
  friend PKGSTATE_API bool operator<(const package_reference& lhs,
                                     const package_reference& rhs) noexcept;

private:
  std::string name_;
};

/*! \brief Canonical selected-profile name including its leading `@`. */
class PKGSTATE_API profile_reference final {
public:
  /*!
   * \brief Construct a validated profile reference.
   * \param name Lowercase profile atom beginning with `@` and a letter.
   * \throws identity_error when \p name is not canonical.
   */
  explicit profile_reference(std::string name);

  /*! \brief Return the canonical profile name including `@`. */
  [[nodiscard]] const std::string& name() const noexcept;

  /*! \brief Compare profile references for equality. */
  friend PKGSTATE_API bool operator==(const profile_reference& lhs,
                                      const profile_reference& rhs) noexcept;
  /*! \brief Compare profile references for inequality. */
  friend PKGSTATE_API bool operator!=(const profile_reference& lhs,
                                      const profile_reference& rhs) noexcept;
  /*! \brief Order profile references lexicographically. */
  friend PKGSTATE_API bool operator<(const profile_reference& lhs,
                                     const profile_reference& rhs) noexcept;

private:
  std::string name_;
};

/*! \brief Canonical build or target architecture name. */
class PKGSTATE_API architecture_reference final {
public:
  /*!
   * \brief Construct a validated architecture reference.
   * \param name Lowercase architecture atom beginning with an ASCII letter.
   * \throws identity_error when \p name is not canonical.
   */
  explicit architecture_reference(std::string name);

  /*! \brief Return the canonical architecture name. */
  [[nodiscard]] const std::string& name() const noexcept;

  /*! \brief Compare architecture references for equality. */
  friend PKGSTATE_API bool operator==(
      const architecture_reference& lhs,
      const architecture_reference& rhs) noexcept;
  /*! \brief Compare architecture references for inequality. */
  friend PKGSTATE_API bool operator!=(
      const architecture_reference& lhs,
      const architecture_reference& rhs) noexcept;
  /*! \brief Order architecture references lexicographically. */
  friend PKGSTATE_API bool operator<(
      const architecture_reference& lhs,
      const architecture_reference& rhs) noexcept;

private:
  std::string name_;
};

/*! \brief Exact declaration site retained as diagnostic provenance. */
class PKGSTATE_API declaration_provenance final {
public:
  /*!
   * \brief Construct declaration provenance.
   * \param document Non-empty single-line source document identifier.
   * \param path Non-empty single-line field or schema path.
   * \param line One-based source line.
   * \param column One-based source column.
   * \throws state_error for unsafe text or zero coordinates.
   */
  declaration_provenance(std::string document,
                         std::string path,
                         std::uint32_t line,
                         std::uint32_t column);

  /*! \brief Return the source document identifier. */
  [[nodiscard]] const std::string& document() const noexcept;
  /*! \brief Return the field or schema path. */
  [[nodiscard]] const std::string& path() const noexcept;
  /*! \brief Return the one-based source line. */
  [[nodiscard]] std::uint32_t line() const noexcept;
  /*! \brief Return the one-based source column. */
  [[nodiscard]] std::uint32_t column() const noexcept;

  /*! \brief Compare complete declaration provenance for equality. */
  friend PKGSTATE_API bool operator==(
      const declaration_provenance& lhs,
      const declaration_provenance& rhs) noexcept;
  /*! \brief Compare complete declaration provenance for inequality. */
  friend PKGSTATE_API bool operator!=(
      const declaration_provenance& lhs,
      const declaration_provenance& rhs) noexcept;
  /*! \brief Order provenance by document, path, line, and column. */
  friend PKGSTATE_API bool operator<(
      const declaration_provenance& lhs,
      const declaration_provenance& rhs) noexcept;

private:
  std::string document_;
  std::string path_;
  std::uint32_t line_;
  std::uint32_t column_;
};

/*! \brief One exact retained step in a source-profile expansion chain. */
class PKGSTATE_API profile_expansion_step final {
public:
  /*!
   * \brief Construct one profile-expansion step.
   * \param profile Profile whose member was traversed.
   * \param member_kind Whether the member names a package or profile.
   * \param member Canonical member text matching \p member_kind.
   * \param provenance Exact member declaration site.
   * \throws state_error when member kind and text disagree.
   */
  profile_expansion_step(profile_reference profile,
                         requirement_member_kind member_kind,
                         std::string member,
                         declaration_provenance provenance);

  /*! \brief Return the profile traversed by this step. */
  [[nodiscard]] const profile_reference& profile() const noexcept;
  /*! \brief Return whether the retained member is a package or profile. */
  [[nodiscard]] requirement_member_kind member_kind() const noexcept;
  /*! \brief Return canonical member text. */
  [[nodiscard]] const std::string& member() const noexcept;
  /*! \brief Return the exact member declaration site. */
  [[nodiscard]] const declaration_provenance& provenance() const noexcept;

  /*! \brief Compare complete expansion steps for equality. */
  friend PKGSTATE_API bool operator==(
      const profile_expansion_step& lhs,
      const profile_expansion_step& rhs) noexcept;
  /*! \brief Compare complete expansion steps for inequality. */
  friend PKGSTATE_API bool operator!=(
      const profile_expansion_step& lhs,
      const profile_expansion_step& rhs) noexcept;
  /*! \brief Order expansion steps canonically. */
  friend PKGSTATE_API bool operator<(
      const profile_expansion_step& lhs,
      const profile_expansion_step& rhs) noexcept;

private:
  profile_reference profile_;
  requirement_member_kind member_kind_;
  std::string member_;
  declaration_provenance provenance_;
};

/*! \brief One direct declaration and its exact profile-expansion path. */
class PKGSTATE_API requirement_origin final {
public:
  /*!
   * \brief Construct one retained requirement origin.
   * \param declaration Direct requirement declaration site.
   * \param expansion Exact profile expansion, empty for direct packages.
   */
  requirement_origin(
      declaration_provenance declaration,
      std::vector<profile_expansion_step> expansion = {});

  /*! \brief Return the direct requirement declaration site. */
  [[nodiscard]] const declaration_provenance& declaration() const noexcept;
  /*! \brief Return the exact profile expansion from declaration to package. */
  [[nodiscard]] const std::vector<profile_expansion_step>&
  expansion() const noexcept;

  /*! \brief Compare complete requirement origins for equality. */
  friend PKGSTATE_API bool operator==(const requirement_origin& lhs,
                                      const requirement_origin& rhs) noexcept;
  /*! \brief Compare complete requirement origins for inequality. */
  friend PKGSTATE_API bool operator!=(const requirement_origin& lhs,
                                      const requirement_origin& rhs) noexcept;
  /*! \brief Order requirement origins canonically. */
  friend PKGSTATE_API bool operator<(const requirement_origin& lhs,
                                     const requirement_origin& rhs) noexcept;

private:
  declaration_provenance declaration_;
  std::vector<profile_expansion_step> expansion_;
};

/*! \brief One resolved package requirement with complete source provenance. */
class PKGSTATE_API package_requirement final {
public:
  /*!
   * \brief Construct a resolved package requirement.
   * \param package Exact required package.
   * \param origins Non-empty issuing declarations, normalized by the class.
   * \throws state_error for no origins, duplicates, or a profile expansion
   * that does not terminate at \p package.
   */
  package_requirement(package_reference package,
                      std::vector<requirement_origin> origins);

  /*! \brief Return the exact required package. */
  [[nodiscard]] const package_reference& package() const noexcept;
  /*! \brief Return canonical issuing origins. */
  [[nodiscard]] const std::vector<requirement_origin>& origins() const noexcept;

  /*! \brief Compare complete package requirements for equality. */
  friend PKGSTATE_API bool operator==(const package_requirement& lhs,
                                      const package_requirement& rhs) noexcept;
  /*! \brief Compare complete package requirements for inequality. */
  friend PKGSTATE_API bool operator!=(const package_requirement& lhs,
                                      const package_requirement& rhs) noexcept;
  /*! \brief Order package requirements canonically. */
  friend PKGSTATE_API bool operator<(const package_requirement& lhs,
                                     const package_requirement& rhs) noexcept;

private:
  package_reference package_;
  std::vector<requirement_origin> origins_;
};

/*! \brief Exact retained lifecycle program. */
class PKGSTATE_API program final {
public:
  /*!
   * \brief Construct exact executable program material.
   * \param language Program language.
   * \param material Non-empty text without NUL or unsafe control bytes.
   * \throws state_error for unsupported language or unsafe material.
   */
  program(program_language language, std::string material);

  /*! \brief Return the retained program language. */
  [[nodiscard]] program_language language() const noexcept;
  /*! \brief Return exact program material. */
  [[nodiscard]] const std::string& material() const noexcept;

  /*! \brief Compare exact programs for equality. */
  friend PKGSTATE_API bool operator==(const program& lhs,
                                      const program& rhs) noexcept;
  /*! \brief Compare exact programs for inequality. */
  friend PKGSTATE_API bool operator!=(const program& lhs,
                                      const program& rhs) noexcept;
  /*! \brief Order exact programs canonically. */
  friend PKGSTATE_API bool operator<(const program& lhs,
                                     const program& rhs) noexcept;

private:
  program_language language_;
  std::string material_;
};

/*! \brief Exact program bound to one lifecycle action. */
class PKGSTATE_API lifecycle_program final {
public:
  /*!
   * \brief Construct an action-bound lifecycle program.
   * \param action Exact lifecycle action.
   * \param value Exact executable program.
   * \throws state_error when \p action is not representable.
   */
  lifecycle_program(lifecycle_action action, program value);

  /*! \brief Return the exact lifecycle action. */
  [[nodiscard]] lifecycle_action action() const noexcept;
  /*! \brief Return exact program material and language. */
  [[nodiscard]] const program& value() const noexcept;

  /*! \brief Compare lifecycle programs for equality. */
  friend PKGSTATE_API bool operator==(const lifecycle_program& lhs,
                                      const lifecycle_program& rhs) noexcept;
  /*! \brief Compare lifecycle programs for inequality. */
  friend PKGSTATE_API bool operator!=(const lifecycle_program& lhs,
                                      const lifecycle_program& rhs) noexcept;
  /*! \brief Order lifecycle programs by action and program. */
  friend PKGSTATE_API bool operator<(const lifecycle_program& lhs,
                                     const lifecycle_program& rhs) noexcept;

private:
  lifecycle_action action_;
  program value_;
};

/*! \brief Resolved package requirement bound to one lifecycle action. */
class PKGSTATE_API lifecycle_requirement final {
public:
  /*!
   * \brief Construct an action-bound package requirement.
   * \param action Exact lifecycle action.
   * \param requirement Resolved package requirement and provenance.
   * \throws state_error when \p action is not representable.
   */
  lifecycle_requirement(lifecycle_action action,
                        package_requirement requirement);

  /*! \brief Return the exact lifecycle action. */
  [[nodiscard]] lifecycle_action action() const noexcept;
  /*! \brief Return the resolved package requirement. */
  [[nodiscard]] const package_requirement& requirement() const noexcept;

  /*! \brief Compare lifecycle requirements for equality. */
  friend PKGSTATE_API bool operator==(
      const lifecycle_requirement& lhs,
      const lifecycle_requirement& rhs) noexcept;
  /*! \brief Compare lifecycle requirements for inequality. */
  friend PKGSTATE_API bool operator!=(
      const lifecycle_requirement& lhs,
      const lifecycle_requirement& rhs) noexcept;
  /*! \brief Order lifecycle requirements canonically. */
  friend PKGSTATE_API bool operator<(
      const lifecycle_requirement& lhs,
      const lifecycle_requirement& rhs) noexcept;

private:
  lifecycle_action action_;
  package_requirement requirement_;
};

/*! \brief Durable user-facing package metadata from sealed source authority. */
class PKGSTATE_API package_metadata final {
public:
  /*!
   * \brief Construct normalized package metadata.
   * \param summary Non-empty single-line summary.
   * \param description Optional non-empty text description.
   * \param homepage Optional non-empty single-line homepage.
   * \param licenses Non-empty license list, normalized by the class.
   * \throws state_error for unsafe text, no licenses, or duplicate licenses.
   */
  package_metadata(std::string summary,
                   std::optional<std::string> description,
                   std::optional<std::string> homepage,
                   std::vector<std::string> licenses);

  /*! \brief Return the package summary. */
  [[nodiscard]] const std::string& summary() const noexcept;
  /*! \brief Return the optional package description. */
  [[nodiscard]] const std::optional<std::string>& description() const noexcept;
  /*! \brief Return the optional package homepage. */
  [[nodiscard]] const std::optional<std::string>& homepage() const noexcept;
  /*! \brief Return canonical package license declarations. */
  [[nodiscard]] const std::vector<std::string>& licenses() const noexcept;

  /*! \brief Compare complete package metadata for equality. */
  friend PKGSTATE_API bool operator==(const package_metadata& lhs,
                                      const package_metadata& rhs) noexcept;
  /*! \brief Compare complete package metadata for inequality. */
  friend PKGSTATE_API bool operator!=(const package_metadata& lhs,
                                      const package_metadata& rhs) noexcept;
  /*! \brief Order package metadata canonically. */
  friend PKGSTATE_API bool operator<(const package_metadata& lhs,
                                     const package_metadata& rhs) noexcept;

private:
  std::string summary_;
  std::optional<std::string> description_;
  std::optional<std::string> homepage_;
  std::vector<std::string> licenses_;
};

/*! \brief One source-selected build profile and all issuing declarations. */
class PKGSTATE_API selected_profile final {
public:
  /*!
   * \brief Construct a selected-profile record.
   * \param profile Exact selected profile.
   * \param identity Source-owned identity of the sealed profile.
   * \param declarations Non-empty issuing declarations, normalized by class.
   * \throws state_error for no declarations or duplicates.
   */
  selected_profile(profile_reference profile,
                   source_profile_identity identity,
                   std::vector<declaration_provenance> declarations);

  /*! \brief Return the selected profile. */
  [[nodiscard]] const profile_reference& profile() const noexcept;
  /*! \brief Return the source-owned profile identity. */
  [[nodiscard]] const source_profile_identity& identity() const noexcept;
  /*! \brief Return canonical issuing declarations. */
  [[nodiscard]] const std::vector<declaration_provenance>&
  declarations() const noexcept;

  /*! \brief Compare selected profiles for equality. */
  friend PKGSTATE_API bool operator==(const selected_profile& lhs,
                                      const selected_profile& rhs) noexcept;
  /*! \brief Compare selected profiles for inequality. */
  friend PKGSTATE_API bool operator!=(const selected_profile& lhs,
                                      const selected_profile& rhs) noexcept;
  /*! \brief Order selected profiles canonically. */
  friend PKGSTATE_API bool operator<(const selected_profile& lhs,
                                     const selected_profile& rhs) noexcept;

private:
  profile_reference profile_;
  source_profile_identity identity_;
  std::vector<declaration_provenance> declarations_;
};

/*! \brief Declared architecture constraints and exact selected pair. */
class PKGSTATE_API architecture_binding final {
public:
  /*!
   * \brief Normalize declarations and bind selected architectures.
   * \param declared_build Permitted build architectures; empty means open.
   * \param declared_target Permitted target architectures; empty means open.
   * \param selected_build Exact architecture used to perform the build.
   * \param selected_target Exact target architecture of the artifact.
   * \return Validated normalized architecture binding.
   * \throws state_error for duplicate declarations or disallowed selections.
   */
  [[nodiscard]] static architecture_binding make(
      std::vector<architecture_reference> declared_build,
      std::vector<architecture_reference> declared_target,
      architecture_reference selected_build,
      architecture_reference selected_target);

  /*! \brief Return canonical declared build architectures. */
  [[nodiscard]] const std::vector<architecture_reference>&
  declared_build() const noexcept;
  /*! \brief Return canonical declared target architectures. */
  [[nodiscard]] const std::vector<architecture_reference>&
  declared_target() const noexcept;
  /*! \brief Return the exact selected build architecture. */
  [[nodiscard]] const architecture_reference& selected_build() const noexcept;
  /*! \brief Return the exact selected target architecture. */
  [[nodiscard]] const architecture_reference& selected_target() const noexcept;

  /*! \brief Compare complete architecture bindings for equality. */
  friend PKGSTATE_API bool operator==(const architecture_binding& lhs,
                                      const architecture_binding& rhs) noexcept;
  /*! \brief Compare complete architecture bindings for inequality. */
  friend PKGSTATE_API bool operator!=(const architecture_binding& lhs,
                                      const architecture_binding& rhs) noexcept;
  /*! \brief Order architecture bindings canonically. */
  friend PKGSTATE_API bool operator<(const architecture_binding& lhs,
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
