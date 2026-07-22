// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file installed_control.h
 * \brief Durable historical control retained for installed packages.
 */

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <libpkgstate/digest.h>
#include <libpkgstate/package_release.h>

namespace pkgstate {

/*!
 * \brief Historical source and completeness of one installed-control fact set.
 *
 * Empty values in the first three states are known empty.  The final state means
 * that the historical storage authority did not retain the fact set at all.
 * There is deliberately no state for facts reconstructed from current mutable
 * source, candidate, archive, or filesystem state.
 */
enum class installed_control_fact_state : std::uint8_t {
  recorded_at_installation = 1,
  recorded_in_compatibility_storage = 2,
  supplied_by_migration = 3,
  historically_unavailable = 4,
};

/*! \brief Return whether a control fact set is historically known. */
[[nodiscard]] bool
is_known(installed_control_fact_state state) noexcept;

/*!
 * \brief One normalized runtime dependency declaration accepted at installation.
 *
 * libpkgstate preserves the declaration text but does not parse, order, or solve
 * dependencies.
 */
class runtime_dependency_declaration final {
public:
  /*! \brief Validate and construct one declaration. */
  [[nodiscard]] static runtime_dependency_declaration
  make(std::string_view expression);

  /*! \brief Return the exact normalized declaration text. */
  [[nodiscard]] const std::string& expression() const noexcept;

  friend bool operator==(const runtime_dependency_declaration& lhs,
                         const runtime_dependency_declaration& rhs) noexcept;
  friend bool operator!=(const runtime_dependency_declaration& lhs,
                         const runtime_dependency_declaration& rhs) noexcept;
  friend bool operator<(const runtime_dependency_declaration& lhs,
                        const runtime_dependency_declaration& rhs) noexcept;

private:
  explicit runtime_dependency_declaration(std::string expression);

  std::string expression_;
};

/*! \brief Removal lifecycle phase retained in installed control. */
enum class removal_lifecycle_phase : std::uint8_t {
  pre_remove = 1,
  post_remove = 2,
};

/*!
 * \brief One exact removal lifecycle declaration.
 *
 * The format is a stable caller-defined executor or media-type identifier.  The
 * material is preserved as exact bytes and may contain NUL or line separators.
 * libpkgstate stores the declaration but does not execute or interpret it.
 */
class removal_lifecycle_declaration final {
public:
  /*! \brief Validate and construct one exact lifecycle declaration. */
  [[nodiscard]] static removal_lifecycle_declaration
  make(removal_lifecycle_phase phase,
       std::string_view format,
       std::string material);

  /*! \brief Return the removal phase. */
  [[nodiscard]] removal_lifecycle_phase phase() const noexcept;

  /*! \brief Return the stable declaration format. */
  [[nodiscard]] const std::string& format() const noexcept;

  /*! \brief Return the exact declaration bytes. */
  [[nodiscard]] const std::string& material() const noexcept;

  friend bool operator==(const removal_lifecycle_declaration& lhs,
                         const removal_lifecycle_declaration& rhs) noexcept;
  friend bool operator!=(const removal_lifecycle_declaration& lhs,
                         const removal_lifecycle_declaration& rhs) noexcept;
  friend bool operator<(const removal_lifecycle_declaration& lhs,
                        const removal_lifecycle_declaration& rhs) noexcept;

private:
  removal_lifecycle_declaration(removal_lifecycle_phase phase,
                                std::string format,
                                std::string material);

  removal_lifecycle_phase phase_;
  std::string format_;
  std::string material_;
};

/*!
 * \brief One target or package-profile fact retained for later reasoning.
 *
 * The state library preserves named values without assigning profile semantics.
 */
class target_profile_fact final {
public:
  /*! \brief Validate and construct one named profile fact. */
  [[nodiscard]] static target_profile_fact
  make(std::string_view name, std::string_view value);

  /*! \brief Return the fact name. */
  [[nodiscard]] const std::string& name() const noexcept;

  /*! \brief Return the fact value. */
  [[nodiscard]] const std::string& value() const noexcept;

  friend bool operator==(const target_profile_fact& lhs,
                         const target_profile_fact& rhs) noexcept;
  friend bool operator!=(const target_profile_fact& lhs,
                         const target_profile_fact& rhs) noexcept;
  friend bool operator<(const target_profile_fact& lhs,
                        const target_profile_fact& rhs) noexcept;

private:
  target_profile_fact(std::string name, std::string value);

  std::string name_;
  std::string value_;
};

/*! \brief Semantic class of one retained external provenance identity. */
enum class control_provenance_kind : std::uint8_t {
  candidate_control = 1,
  artifact = 2,
  artifact_manifest = 3,
  application_evidence = 4,
  transaction_evidence = 5,
  legacy_package_observation = 6,
  legacy_snapshot_observation = 7,
  legacy_migration_evidence = 8,
};

/*!
 * \brief One retained external provenance identity.
 *
 * The identity is validated and stored in canonical algorithm-qualified form.
 * Its authority remains the component that originally computed it.
 */
class control_provenance final {
public:
  /*! \brief Validate and construct one provenance reference. */
  [[nodiscard]] static control_provenance
  make(control_provenance_kind kind, std::string_view identity);

  /*! \brief Return the provenance class. */
  [[nodiscard]] control_provenance_kind kind() const noexcept;

  /*! \brief Return canonical algorithm-qualified identity text. */
  [[nodiscard]] const std::string& identity() const noexcept;

  friend bool operator==(const control_provenance& lhs,
                         const control_provenance& rhs) noexcept;
  friend bool operator!=(const control_provenance& lhs,
                         const control_provenance& rhs) noexcept;
  friend bool operator<(const control_provenance& lhs,
                        const control_provenance& rhs) noexcept;

private:
  control_provenance(control_provenance_kind kind, std::string identity);

  control_provenance_kind kind_;
  std::string identity_;
};

/*! \brief Per-group completeness and historical source for installed control. */
struct installed_control_completeness final {
  installed_control_fact_state runtime_dependencies =
      installed_control_fact_state::recorded_at_installation;
  installed_control_fact_state removal_lifecycle =
      installed_control_fact_state::recorded_at_installation;
  installed_control_fact_state target_profile =
      installed_control_fact_state::recorded_at_installation;
  installed_control_fact_state provenance =
      installed_control_fact_state::recorded_at_installation;
};

[[nodiscard]] bool
operator==(const installed_control_completeness& lhs,
           const installed_control_completeness& rhs) noexcept;
[[nodiscard]] bool
operator!=(const installed_control_completeness& lhs,
           const installed_control_completeness& rhs) noexcept;
[[nodiscard]] bool
operator<(const installed_control_completeness& lhs,
          const installed_control_completeness& rhs) noexcept;

/*!
 * \brief Immutable durable control for one installed package release.
 *
 * The object retains historical non-payload facts needed after the original
 * source provider, candidate, and archive have disappeared.  It is separate
 * from installed ownership and does not execute lifecycle declarations, solve
 * dependencies, inspect archives, or reconstruct unavailable facts.
 */
class installed_control final {
public:
  /*! \brief Validate, normalize, identify, and construct installed control. */
  [[nodiscard]] static installed_control
  make(package_release release,
       installed_control_completeness completeness,
       std::vector<runtime_dependency_declaration> runtime_dependencies,
       std::vector<removal_lifecycle_declaration> removal_lifecycle,
       std::vector<target_profile_fact> target_profile,
       std::vector<control_provenance> provenance);

  /*! \brief Return the canonical installed-control identity. */
  [[nodiscard]] const installed_control_identity& identity() const noexcept;

  /*! \brief Return the exact package release to which this control belongs. */
  [[nodiscard]] const package_release& release() const noexcept;

  /*! \brief Return per-group historical completeness. */
  [[nodiscard]] const installed_control_completeness&
  completeness() const noexcept;

  /*! \brief Return declarations in deterministic set order. */
  [[nodiscard]] const std::vector<runtime_dependency_declaration>&
  runtime_dependencies() const noexcept;

  /*! \brief Return pre-remove declarations followed by post-remove declarations. */
  [[nodiscard]] const std::vector<removal_lifecycle_declaration>&
  removal_lifecycle() const noexcept;

  /*! \brief Return profile facts in fact-name order. */
  [[nodiscard]] const std::vector<target_profile_fact>&
  target_profile() const noexcept;

  /*! \brief Return retained provenance in provenance-kind order. */
  [[nodiscard]] const std::vector<control_provenance>&
  provenance() const noexcept;

  friend bool operator==(const installed_control& lhs,
                         const installed_control& rhs) noexcept;
  friend bool operator!=(const installed_control& lhs,
                         const installed_control& rhs) noexcept;
  friend bool operator<(const installed_control& lhs,
                        const installed_control& rhs) noexcept;

private:
  installed_control(installed_control_identity identity,
                    package_release release,
                    installed_control_completeness completeness,
                    std::vector<runtime_dependency_declaration>
                        runtime_dependencies,
                    std::vector<removal_lifecycle_declaration>
                        removal_lifecycle,
                    std::vector<target_profile_fact> target_profile,
                    std::vector<control_provenance> provenance);

  installed_control_identity identity_;
  package_release release_;
  installed_control_completeness completeness_;
  std::vector<runtime_dependency_declaration> runtime_dependencies_;
  std::vector<removal_lifecycle_declaration> removal_lifecycle_;
  std::vector<target_profile_fact> target_profile_;
  std::vector<control_provenance> provenance_;
};

} // namespace pkgstate
