// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file installed_control.h
 *  \brief Complete durable non-payload control for an installed package.
 */
#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <libpkgstate/digest.h>
#include <libpkgstate/model.h>
#include <libpkgstate/package_source_record.h>

namespace pkgstate {

enum class installation_reason_kind : std::uint8_t {
  explicit_request = 1,
  runtime_dependency = 2,
  profile_membership = 3,
  system_policy = 4,
};

class installation_reason final {
public:
  [[nodiscard]] static installation_reason explicit_request();
  [[nodiscard]] static installation_reason runtime_dependency(package_reference issuer);
  [[nodiscard]] static installation_reason profile_membership(
      profile_reference profile, source_profile_identity identity);
  [[nodiscard]] static installation_reason system_policy(std::string policy);

  [[nodiscard]] installation_reason_kind kind() const noexcept;
  [[nodiscard]] const std::optional<package_reference>& issuer_package() const noexcept;
  [[nodiscard]] const std::optional<profile_reference>& issuer_profile() const noexcept;
  [[nodiscard]] const std::optional<source_profile_identity>& issuer_profile_identity() const noexcept;
  [[nodiscard]] const std::optional<std::string>& policy() const noexcept;

  friend bool operator==(const installation_reason& lhs,
                         const installation_reason& rhs) noexcept;
  friend bool operator!=(const installation_reason& lhs,
                         const installation_reason& rhs) noexcept;
  friend bool operator<(const installation_reason& lhs,
                        const installation_reason& rhs) noexcept;
private:
  installation_reason(installation_reason_kind kind,
                      std::optional<package_reference> issuer_package,
                      std::optional<profile_reference> issuer_profile,
                      std::optional<source_profile_identity> issuer_profile_identity,
                      std::optional<std::string> policy);
  installation_reason_kind kind_;
  std::optional<package_reference> issuer_package_;
  std::optional<profile_reference> issuer_profile_;
  std::optional<source_profile_identity> issuer_profile_identity_;
  std::optional<std::string> policy_;
};

class build_provenance final {
public:
  build_provenance(
      package_source_record_identity source_record,
      build_request_identity request,
      source_material_set_identity source_materials,
      build_input_set_identity build_inputs,
      environment_policy_identity environment_policy,
      build_policy_identity build_policy,
      build_result_identity build_result,
      payload_manifest_identity payload_manifest,
      build_artifact_identity artifact,
      artifact_content_identity artifact_content,
      artifact_binding_identity artifact_binding,
      execution_evidence_identity execution_evidence,
      artifact_image_identity artifact_image,
      artifact_inspection_identity artifact_inspection);

  [[nodiscard]] const package_source_record_identity& source_record() const noexcept;
  [[nodiscard]] const build_request_identity& request() const noexcept;
  [[nodiscard]] const source_material_set_identity& source_materials() const noexcept;
  [[nodiscard]] const build_input_set_identity& build_inputs() const noexcept;
  [[nodiscard]] const environment_policy_identity& environment_policy() const noexcept;
  [[nodiscard]] const build_policy_identity& build_policy() const noexcept;
  [[nodiscard]] const build_result_identity& build_result() const noexcept;
  [[nodiscard]] const payload_manifest_identity& payload_manifest() const noexcept;
  [[nodiscard]] const build_artifact_identity& artifact() const noexcept;
  [[nodiscard]] const artifact_content_identity& artifact_content() const noexcept;
  [[nodiscard]] const artifact_binding_identity& artifact_binding() const noexcept;
  [[nodiscard]] const execution_evidence_identity& execution_evidence() const noexcept;
  [[nodiscard]] const artifact_image_identity& artifact_image() const noexcept;
  [[nodiscard]] const artifact_inspection_identity& artifact_inspection() const noexcept;

  friend bool operator==(const build_provenance& lhs,
                         const build_provenance& rhs) noexcept;
  friend bool operator!=(const build_provenance& lhs,
                         const build_provenance& rhs) noexcept;
  friend bool operator<(const build_provenance& lhs,
                        const build_provenance& rhs) noexcept;
private:
  package_source_record_identity source_record_;
  build_request_identity request_;
  source_material_set_identity source_materials_;
  build_input_set_identity build_inputs_;
  environment_policy_identity environment_policy_;
  build_policy_identity build_policy_;
  build_result_identity build_result_;
  payload_manifest_identity payload_manifest_;
  build_artifact_identity artifact_;
  artifact_content_identity artifact_content_;
  artifact_binding_identity artifact_binding_;
  execution_evidence_identity execution_evidence_;
  artifact_image_identity artifact_image_;
  artifact_inspection_identity artifact_inspection_;
};

class installed_control final {
public:
  [[nodiscard]] static installed_control make(
      package_source_record source,
      installation_reason reason,
      build_provenance build);

  [[nodiscard]] const installed_control_identity& identity() const noexcept;
  [[nodiscard]] const package_source_record& source() const noexcept;
  [[nodiscard]] const package_release& release() const noexcept;
  [[nodiscard]] const installation_reason& reason() const noexcept;
  [[nodiscard]] const build_provenance& build() const noexcept;

  friend bool operator==(const installed_control& lhs,
                         const installed_control& rhs) noexcept;
  friend bool operator!=(const installed_control& lhs,
                         const installed_control& rhs) noexcept;
  friend bool operator<(const installed_control& lhs,
                        const installed_control& rhs) noexcept;
private:
  installed_control(installed_control_identity identity,
                    package_source_record source,
                    installation_reason reason,
                    build_provenance build);
  installed_control_identity identity_;
  package_source_record source_;
  installation_reason reason_;
  build_provenance build_;
};

} // namespace pkgstate
