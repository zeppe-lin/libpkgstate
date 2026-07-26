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
  build_provenance(candidate_control_identity candidate_control,
                   build_input_set_identity build_inputs,
                   build_result_identity build_result,
                   artifact_identity artifact,
                   artifact_manifest_identity artifact_manifest);
  [[nodiscard]] const candidate_control_identity& candidate_control() const noexcept;
  [[nodiscard]] const build_input_set_identity& build_inputs() const noexcept;
  [[nodiscard]] const build_result_identity& build_result() const noexcept;
  [[nodiscard]] const artifact_identity& artifact() const noexcept;
  [[nodiscard]] const artifact_manifest_identity& artifact_manifest() const noexcept;
  friend bool operator==(const build_provenance& lhs,
                         const build_provenance& rhs) noexcept;
  friend bool operator!=(const build_provenance& lhs,
                         const build_provenance& rhs) noexcept;
  friend bool operator<(const build_provenance& lhs,
                        const build_provenance& rhs) noexcept;
private:
  candidate_control_identity candidate_control_;
  build_input_set_identity build_inputs_;
  build_result_identity build_result_;
  artifact_identity artifact_;
  artifact_manifest_identity artifact_manifest_;
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
