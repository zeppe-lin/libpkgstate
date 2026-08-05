// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file installed_control.h
 * \brief Complete durable non-payload control for one installed package.
 */
#pragma once

#include <libpkgstate/export.h>

#include <cstdint>
#include <optional>
#include <string>

#include <libpkgstate/digest.h>
#include <libpkgstate/model.h>
#include <libpkgstate/package_source_record.h>

namespace pkgstate {

/*! \brief Authority that caused one package to become installed. */
enum class installation_reason_kind : std::uint8_t {
  explicit_request = 1,   //!< A caller explicitly selected this package.
  runtime_dependency = 2, //!< Another installed package required it at runtime.
  profile_membership = 3, //!< A selected source profile included it.
  system_policy = 4,      //!< A named system policy selected it.
};

/*!
 * \brief Typed installation reason with exactly one matching payload.
 *
 * The reason records selection authority, not a reconstruction heuristic. A
 * package manager may use it for future reconciliation, but libpkgstate does
 * not infer or rewrite reasons after publication.
 */
class PKGSTATE_API installation_reason final {
public:
  /*!
   * \brief Construct an explicit caller request without issuer payload.
   * \return Constructed validated value.
   */
  [[nodiscard]] static installation_reason explicit_request();

  /*!
   * \brief Construct a runtime-dependency reason.
   * \param issuer Exact installed package whose runtime closure selected this.
   * \return Constructed validated value.
   */
  [[nodiscard]] static installation_reason
  runtime_dependency(package_reference issuer);

  /*!
   * \brief Construct a source-profile membership reason.
   * \param profile Exact selected profile.
   * \param identity Source-owned identity of the sealed profile.
   * \return Constructed validated value.
   */
  [[nodiscard]] static installation_reason profile_membership(
      profile_reference profile,
      source_profile_identity identity);

  /*!
   * \brief Construct a named system-policy reason.
   * \param policy Non-empty single-line policy identifier.
   * \throws state_error when \p policy is unsafe.
   * \return Constructed validated value.
   */
  [[nodiscard]] static installation_reason system_policy(std::string policy);

  /*!
   * \brief Return the active reason kind.
   * \return The active reason kind.
   */
  [[nodiscard]] installation_reason_kind kind() const noexcept;
  /*!
   * \brief Return issuer package only for runtime_dependency.
   * \return Issuer package only for runtime_dependency.
   */
  [[nodiscard]] const std::optional<package_reference>&
  issuer_package() const noexcept;
  /*!
   * \brief Return issuer profile only for profile_membership.
   * \return Issuer profile only for profile_membership.
   */
  [[nodiscard]] const std::optional<profile_reference>&
  issuer_profile() const noexcept;
  /*!
   * \brief Return issuer profile identity only for profile_membership.
   * \return Issuer profile identity only for profile_membership.
   */
  [[nodiscard]] const std::optional<source_profile_identity>&
  issuer_profile_identity() const noexcept;
  /*!
   * \brief Return policy identifier only for system_policy.
   * \return Policy identifier only for system_policy.
   */
  [[nodiscard]] const std::optional<std::string>& policy() const noexcept;

  /*!
   * \brief Compare complete installation reasons for equality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands are equal.
   */
  friend PKGSTATE_API bool operator==(const installation_reason& lhs,
                                      const installation_reason& rhs) noexcept;
  /*!
   * \brief Compare complete installation reasons for inequality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands differ.
   */
  friend PKGSTATE_API bool operator!=(const installation_reason& lhs,
                                      const installation_reason& rhs) noexcept;
  /*!
   * \brief Order installation reasons canonically.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the left operand precedes the right operand.
   */
  friend PKGSTATE_API bool operator<(const installation_reason& lhs,
                                     const installation_reason& rhs) noexcept;

private:
  installation_reason(
      installation_reason_kind kind,
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

/*!
 * \brief Exact foreign build authority retained beside installed state.
 *
 * Every field is an identity issued by the semantic owner named in that
 * field. libpkgstate stores the binding; it does not recompute build, image,
 * or execution identities.
 */
class PKGSTATE_API build_provenance final {
public:
  /*!
   * \brief Construct complete build provenance.
   * \param source_record Native identity of the admitted source record.
   * \param request Build-owned request identity.
   * \param build_inputs Build-owned exact input-set identity.
   * \param environment_policy Build-environment policy identity.
   * \param build_policy Build-policy identity.
   * \param build_result Complete build-result identity.
   * \param payload_manifest Build payload-manifest identity.
   * \param artifact Exact artifact authority identity.
   * \param artifact_content Exact artifact-byte identity.
   * \param artifact_binding Artifact authority-to-content binding identity.
   * \param execution_evidence Build-execution evidence identity.
   * \param build_image Build-to-inspected-image admission identity.
   * \param artifact_image Normalized package-image identity.
   * \param artifact_inspection Image-inspection receipt identity.
   */
  build_provenance(
      package_source_record_identity source_record,
      build_request_identity request,
      build_input_set_identity build_inputs,
      environment_policy_identity environment_policy,
      build_policy_identity build_policy,
      build_result_identity build_result,
      payload_manifest_identity payload_manifest,
      build_artifact_identity artifact,
      artifact_content_identity artifact_content,
      artifact_binding_identity artifact_binding,
      execution_evidence_identity execution_evidence,
      build_image_identity build_image,
      artifact_image_identity artifact_image,
      artifact_inspection_identity artifact_inspection);

  /*!
   * \brief Return the admitted native source-record identity.
   * \return The admitted native source-record identity.
   */
  [[nodiscard]] const package_source_record_identity&
  source_record() const noexcept;
  /*!
   * \brief Return the exact build-request identity.
   * \return The exact build-request identity.
   */
  [[nodiscard]] const build_request_identity& request() const noexcept;
  /*!
   * \brief Return the exact build-input-set identity.
   * \return The exact build-input-set identity.
   */
  [[nodiscard]] const build_input_set_identity& build_inputs() const noexcept;
  /*!
   * \brief Return the environment-policy identity.
   * \return The environment-policy identity.
   */
  [[nodiscard]] const environment_policy_identity&
  environment_policy() const noexcept;
  /*!
   * \brief Return the build-policy identity.
   * \return The build-policy identity.
   */
  [[nodiscard]] const build_policy_identity& build_policy() const noexcept;
  /*!
   * \brief Return the complete build-result identity.
   * \return The complete build-result identity.
   */
  [[nodiscard]] const build_result_identity& build_result() const noexcept;
  /*!
   * \brief Return the payload-manifest identity.
   * \return The payload-manifest identity.
   */
  [[nodiscard]] const payload_manifest_identity&
  payload_manifest() const noexcept;
  /*!
   * \brief Return the exact artifact authority identity.
   * \return The exact artifact authority identity.
   */
  [[nodiscard]] const build_artifact_identity& artifact() const noexcept;
  /*!
   * \brief Return the exact artifact-content identity.
   * \return The exact artifact-content identity.
   */
  [[nodiscard]] const artifact_content_identity&
  artifact_content() const noexcept;
  /*!
   * \brief Return the artifact-binding identity.
   * \return The artifact-binding identity.
   */
  [[nodiscard]] const artifact_binding_identity&
  artifact_binding() const noexcept;
  /*!
   * \brief Return build-execution evidence identity.
   * \return Build-execution evidence identity.
   */
  [[nodiscard]] const execution_evidence_identity&
  execution_evidence() const noexcept;
  /*!
   * \brief Return build-to-inspected-image admission identity.
   * \return Build-to-inspected-image admission identity.
   */
  [[nodiscard]] const build_image_identity& build_image() const noexcept;
  /*!
   * \brief Return normalized package-image identity.
   * \return Normalized package-image identity.
   */
  [[nodiscard]] const artifact_image_identity& artifact_image() const noexcept;
  /*!
   * \brief Return image-inspection receipt identity.
   * \return Image-inspection receipt identity.
   */
  [[nodiscard]] const artifact_inspection_identity&
  artifact_inspection() const noexcept;

  /*!
   * \brief Compare complete build provenance for equality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands are equal.
   */
  friend PKGSTATE_API bool operator==(const build_provenance& lhs,
                                      const build_provenance& rhs) noexcept;
  /*!
   * \brief Compare complete build provenance for inequality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands differ.
   */
  friend PKGSTATE_API bool operator!=(const build_provenance& lhs,
                                      const build_provenance& rhs) noexcept;
  /*!
   * \brief Order build provenance canonically.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the left operand precedes the right operand.
   */
  friend PKGSTATE_API bool operator<(const build_provenance& lhs,
                                     const build_provenance& rhs) noexcept;

private:
  package_source_record_identity source_record_;
  build_request_identity request_;
  build_input_set_identity build_inputs_;
  environment_policy_identity environment_policy_;
  build_policy_identity build_policy_;
  build_result_identity build_result_;
  payload_manifest_identity payload_manifest_;
  build_artifact_identity artifact_;
  artifact_content_identity artifact_content_;
  artifact_binding_identity artifact_binding_;
  execution_evidence_identity execution_evidence_;
  build_image_identity build_image_;
  artifact_image_identity artifact_image_;
  artifact_inspection_identity artifact_inspection_;
};

/*! \brief Complete durable source, selection, and build control. */
class PKGSTATE_API installed_control final {
public:
  /*!
   * \brief Validate, normalize, and identify installed control.
   * \param source Complete admitted package-source record.
   * \param reason Exact authority that selected the package.
   * \param build Complete source-bound build provenance.
   * \return Immutable identified installed control.
   * \throws state_error when build provenance names another source record.
   */
  [[nodiscard]] static installed_control make(
      package_source_record source,
      installation_reason reason,
      build_provenance build);

  /*!
   * \brief Return canonical installed-control identity.
   * \return Canonical installed-control identity.
   */
  [[nodiscard]] const installed_control_identity& identity() const noexcept;
  /*!
   * \brief Return complete durable source authority.
   * \return Complete durable source authority.
   */
  [[nodiscard]] const package_source_record& source() const noexcept;
  /*!
   * \brief Return source-authoritative package release.
   * \return Source-authoritative package release.
   */
  [[nodiscard]] const package_release& release() const noexcept;
  /*!
   * \brief Return exact installation reason.
   * \return Exact installation reason.
   */
  [[nodiscard]] const installation_reason& reason() const noexcept;
  /*!
   * \brief Return complete build provenance.
   * \return Complete build provenance.
   */
  [[nodiscard]] const build_provenance& build() const noexcept;

  /*!
   * \brief Compare complete installed control for equality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands are equal.
   */
  friend PKGSTATE_API bool operator==(const installed_control& lhs,
                                      const installed_control& rhs) noexcept;
  /*!
   * \brief Compare complete installed control for inequality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands differ.
   */
  friend PKGSTATE_API bool operator!=(const installed_control& lhs,
                                      const installed_control& rhs) noexcept;
  /*!
   * \brief Order installed control canonically.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the left operand precedes the right operand.
   */
  friend PKGSTATE_API bool operator<(const installed_control& lhs,
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
