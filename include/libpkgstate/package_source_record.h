// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file package_source_record.h
 *  \brief Durable projection of sealed native package-source authority.
 */
#pragma once

#include <vector>

#include <libpkgstate/digest.h>
#include <libpkgstate/model.h>
#include <libpkgstate/package_release.h>

namespace pkgstate {

class package_source_record final {
public:
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

  [[nodiscard]] const package_source_record_identity& identity() const noexcept;
  [[nodiscard]] const package_release& release() const noexcept;
  [[nodiscard]] const package_metadata& metadata() const noexcept;
  [[nodiscard]] const std::vector<package_requirement>& runtime_requirements() const noexcept;
  [[nodiscard]] const std::vector<lifecycle_program>& lifecycle_programs() const noexcept;
  [[nodiscard]] const lifecycle_program* lifecycle(lifecycle_action action) const noexcept;
  [[nodiscard]] const std::vector<lifecycle_requirement>& lifecycle_requirements() const noexcept;
  [[nodiscard]] std::vector<package_requirement> lifecycle_requirements(lifecycle_action action) const;
  [[nodiscard]] const architecture_binding& architectures() const noexcept;
  [[nodiscard]] const std::vector<selected_profile>& selected_profiles() const noexcept;
  [[nodiscard]] const source_recipe_identity& recipe() const noexcept;
  [[nodiscard]] const source_snapshot_identity& snapshot() const noexcept;

  friend bool operator==(const package_source_record& lhs,
                         const package_source_record& rhs) noexcept;
  friend bool operator!=(const package_source_record& lhs,
                         const package_source_record& rhs) noexcept;
  friend bool operator<(const package_source_record& lhs,
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
