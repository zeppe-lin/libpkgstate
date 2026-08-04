// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file package_source_record.h
 * \brief Durable projection of sealed native package-source authority.
 */
#pragma once

#include <libpkgstate/export.h>

#include <vector>

#include <libpkgstate/digest.h>
#include <libpkgstate/model.h>
#include <libpkgstate/package_release.h>

namespace pkgstate {

/*!
 * \brief Complete state-owned record of admitted package source authority.
 *
 * The record retains only durable source facts required after planning and
 * installation: package coordinates and metadata, runtime and lifecycle
 * control, architecture binding, selected profile evidence, and foreign source
 * identities. Source inputs and executable build/check programs remain owned
 * by the source and build authorities and are not duplicated here.
 */
class PKGSTATE_API package_source_record final {
public:
  /*!
   * \brief Normalize, validate, and identify one package source record.
   * \param release Source-authoritative package release.
   * \param metadata Durable package metadata.
   * \param runtime_requirements Runtime package requirements.
   * \param lifecycle_programs Action-bound lifecycle programs.
   * \param lifecycle_requirements Action-bound package requirements.
   * \param architectures Declared and selected architecture authority.
   * \param selected_profiles Exact selected profile evidence.
   * \param recipe Source-owned sealed recipe identity.
   * \param snapshot Source-owned sealed snapshot identity.
   * \return Canonically ordered immutable source record.
   * \throws state_error for duplicate runtime packages, lifecycle actions,
   * action/package requirement pairs, or selected profiles.
   */
  [[nodiscard]] static package_source_record make(
      package_release release,
      package_metadata metadata,
      std::vector<package_requirement> runtime_requirements,
      std::vector<lifecycle_program> lifecycle_programs,
      std::vector<lifecycle_requirement> lifecycle_requirements,
      architecture_binding architectures,
      std::vector<selected_profile> selected_profiles,
      source_recipe_identity recipe,
      source_snapshot_identity snapshot);

  /*!
   * \brief Return the canonical native source-record identity.
   * \return The canonical native source-record identity.
   */
  [[nodiscard]] const package_source_record_identity& identity() const noexcept;
  /*!
   * \brief Return source-authoritative package release coordinates.
   * \return Source-authoritative package release coordinates.
   */
  [[nodiscard]] const package_release& release() const noexcept;
  /*!
   * \brief Return durable package metadata.
   * \return Durable package metadata.
   */
  [[nodiscard]] const package_metadata& metadata() const noexcept;
  /*!
   * \brief Return canonical runtime requirements.
   * \return Canonical runtime requirements.
   */
  [[nodiscard]] const std::vector<package_requirement>&
  runtime_requirements() const noexcept;
  /*!
   * \brief Return canonical action-bound lifecycle programs.
   * \return Canonical action-bound lifecycle programs.
   */
  [[nodiscard]] const std::vector<lifecycle_program>&
  lifecycle_programs() const noexcept;

  /*!
   * \brief Find the program for one lifecycle action.
   * \param action Exact lifecycle action.
   * \return Pointer valid for this record's lifetime, or `nullptr` when no
   * program is retained for \p action.
   */
  [[nodiscard]] const lifecycle_program*
  lifecycle(lifecycle_action action) const noexcept;

  /*!
   * \brief Return all canonical action-bound lifecycle requirements.
   * \return All canonical action-bound lifecycle requirements.
   */
  [[nodiscard]] const std::vector<lifecycle_requirement>&
  lifecycle_requirements() const noexcept;

  /*!
   * \brief Select requirements for one lifecycle action.
   * \param action Exact lifecycle action.
   * \return Matching package requirements in canonical order.
   */
  [[nodiscard]] std::vector<package_requirement>
  lifecycle_requirements(lifecycle_action action) const;

  /*!
   * \brief Return declared and selected architecture authority.
   * \return Declared and selected architecture authority.
   */
  [[nodiscard]] const architecture_binding& architectures() const noexcept;
  /*!
   * \brief Return exact selected profile evidence.
   * \return Exact selected profile evidence.
   */
  [[nodiscard]] const std::vector<selected_profile>&
  selected_profiles() const noexcept;
  /*!
   * \brief Return the foreign sealed-recipe identity.
   * \return The foreign sealed-recipe identity.
   */
  [[nodiscard]] const source_recipe_identity& recipe() const noexcept;
  /*!
   * \brief Return the foreign sealed-source-snapshot identity.
   * \return The foreign sealed-source-snapshot identity.
   */
  [[nodiscard]] const source_snapshot_identity& snapshot() const noexcept;

  /*!
   * \brief Compare complete source records for equality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands are equal.
   */
  friend PKGSTATE_API bool operator==(const package_source_record& lhs,
                                      const package_source_record& rhs) noexcept;
  /*!
   * \brief Compare complete source records for inequality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands differ.
   */
  friend PKGSTATE_API bool operator!=(const package_source_record& lhs,
                                      const package_source_record& rhs) noexcept;
  /*!
   * \brief Order source records by canonical identity.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the left operand precedes the right operand.
   */
  friend PKGSTATE_API bool operator<(const package_source_record& lhs,
                                     const package_source_record& rhs) noexcept;

private:
  package_source_record(package_source_record_identity identity,
                        package_release release,
                        package_metadata metadata,
                        std::vector<package_requirement> runtime_requirements,
                        std::vector<lifecycle_program> lifecycle_programs,
                        std::vector<lifecycle_requirement> lifecycle_requirements,
                        architecture_binding architectures,
                        std::vector<selected_profile> selected_profiles,
                        source_recipe_identity recipe,
                        source_snapshot_identity snapshot);

  package_source_record_identity identity_;
  package_release release_;
  package_metadata metadata_;
  std::vector<package_requirement> runtime_requirements_;
  std::vector<lifecycle_program> lifecycle_programs_;
  std::vector<lifecycle_requirement> lifecycle_requirements_;
  architecture_binding architectures_;
  std::vector<selected_profile> selected_profiles_;
  source_recipe_identity recipe_;
  source_snapshot_identity snapshot_;
};

} // namespace pkgstate
