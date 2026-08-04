// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file digest.h
 * \brief Strongly typed installed-state identities and foreign references.
 */
#pragma once

#include <libpkgstate/export.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <libpkgstate/error.h>

namespace pkgstate {

/*! \brief Version of the public algorithm-qualified digest representation. */
inline constexpr std::uint16_t digest_representation_version = 1;

/*! \brief Digest algorithm carried by native state identity values. */
enum class digest_algorithm : std::uint16_t {
  sha256 = 1, //!< SHA-256 with a 32-byte result.
};

/*! \brief Number of bytes in one SHA-256 result. */
inline constexpr std::size_t sha256_digest_size = 32;

/*! \brief Algorithm-neutral storage for a digest result. */
using digest_bytes = std::vector<std::uint8_t>;

/*! \brief Fixed-size SHA-256 result accepted by construction factories. */
using sha256_digest_bytes = std::array<std::uint8_t, sha256_digest_size>;

/*! \brief Stable reason that a canonical digest representation was refused. */
enum class digest_error_code {
  invalid_format,        //!< The representation does not contain three fields.
  unsupported_version,   //!< The representation version is not supported.
  unsupported_algorithm, //!< The algorithm tag is not supported.
  invalid_length,        //!< The hexadecimal payload has the wrong length.
  invalid_hex,           //!< The payload is not lowercase hexadecimal text.
};

/*! \brief Failure to construct one native or referenced identity value. */
class PKGSTATE_API digest_error final : public identity_error {
public:
  /*!
   * \brief Construct a typed digest failure.
   * \param code Stable refusal category.
   * \param message Human-readable diagnostic text.
   */
  digest_error(digest_error_code code, std::string message);

  /*! \brief Destroy the polymorphic digest failure. */
  ~digest_error() override;

  /*!
   * \brief Return the stable refusal category.
   * \return The stable refusal category.
   */
  [[nodiscard]] digest_error_code code() const noexcept;

private:
  digest_error_code code_;
};

namespace detail {

/*!
 * \brief Shared representation mechanics behind every state digest domain.
 *
 * This type has no semantic identity of its own. Public callers use the
 * strongly typed aliases declared below; the shared value exists only to keep
 * parsing, formatting, and comparison rules identical between domains.
 */
class PKGSTATE_API digest_value final {
public:
  /*!
   * \brief Construct a SHA-256 representation from exact digest bytes.
   * \param bytes SHA-256 result bytes in network-independent order.
   * \return Value using representation version 1 and the SHA-256 algorithm.
   */
  [[nodiscard]] static digest_value from_sha256(sha256_digest_bytes bytes);

  /*!
   * \brief Parse one canonical digest representation.
   * \param input Text in `v1:sha256:<lowercase-hex>` form.
   * \return Parsed digest value.
   * \throws digest_error when the representation is malformed or unsupported.
   */
  [[nodiscard]] static digest_value parse(std::string_view input);

  /*!
   * \brief Return the represented wire-format version.
   * \return The represented wire-format version.
   */
  [[nodiscard]] std::uint16_t representation_version() const noexcept;

  /*!
   * \brief Return the represented digest algorithm.
   * \return The represented digest algorithm.
   */
  [[nodiscard]] digest_algorithm algorithm() const noexcept;

  /*!
   * \brief Return the exact digest result bytes.
   * \return The exact digest result bytes.
   */
  [[nodiscard]] const digest_bytes& bytes() const noexcept;

  /*!
   * \brief Return canonical `v1:sha256:<lowercase-hex>` text.
   * \return Canonical `v1:sha256:<lowercase-hex>` text.
   */
  [[nodiscard]] std::string string() const;

  /*!
   * \brief Compare complete represented values for equality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands are equal.
   */
  friend PKGSTATE_API bool operator==(const digest_value& lhs,
                                      const digest_value& rhs) noexcept;

  /*!
   * \brief Compare complete represented values for inequality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands differ.
   */
  friend PKGSTATE_API bool operator!=(const digest_value& lhs,
                                      const digest_value& rhs) noexcept;

  /*!
   * \brief Order represented values by version, algorithm, and bytes.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the left operand precedes the right operand.
   */
  friend PKGSTATE_API bool operator<(const digest_value& lhs,
                                     const digest_value& rhs) noexcept;

private:
  digest_value(std::uint16_t representation_version,
               digest_algorithm algorithm,
               digest_bytes bytes);

  std::uint16_t representation_version_;
  digest_algorithm algorithm_;
  digest_bytes bytes_;
};

/*!
 * \brief Strong wrapper for one identity computed and owned by libpkgstate.
 * \tparam Domain Empty tag naming one native identity protocol domain.
 *
 * Different specializations are intentionally non-convertible and
 * non-comparable even though they share the same digest representation.
 */
template<typename Domain>
class PKGSTATE_API typed_digest final {
public:
  /*!
   * \brief Construct this identity domain from exact SHA-256 bytes.
   * \param bytes Exact SHA-256 result bytes.
   * \return Typed digest containing the supplied SHA-256 bytes.
   */
  [[nodiscard]] static typed_digest from_sha256(sha256_digest_bytes bytes)
  {
    return typed_digest(digest_value::from_sha256(std::move(bytes)));
  }

  /*!
   * \brief Parse canonical text into this exact identity domain.
   * \param input Canonical textual representation to parse.
   * \return Parsed canonical digest value.
   */
  [[nodiscard]] static typed_digest parse(std::string_view input)
  {
    return typed_digest(digest_value::parse(input));
  }

  /*!
   * \brief Return the canonical protocol-domain identifier.
   * \return The canonical protocol-domain identifier.
   */
  [[nodiscard]] static constexpr std::string_view canonical_domain() noexcept
  {
    return Domain::canonical_domain;
  }

  /*!
   * \brief Return the represented wire-format version.
   * \return The represented wire-format version.
   */
  [[nodiscard]] std::uint16_t representation_version() const noexcept
  {
    return value_.representation_version();
  }

  /*!
   * \brief Return the represented digest algorithm.
   * \return The represented digest algorithm.
   */
  [[nodiscard]] digest_algorithm algorithm() const noexcept
  {
    return value_.algorithm();
  }

  /*!
   * \brief Return exact digest result bytes.
   * \return Exact digest result bytes.
   */
  [[nodiscard]] const digest_bytes& bytes() const noexcept
  {
    return value_.bytes();
  }

  /*!
   * \brief Return canonical algorithm-qualified text.
   * \return Canonical algorithm-qualified text.
   */
  [[nodiscard]] std::string string() const { return value_.string(); }

  /*!
   * \brief Compare values in this exact identity domain for equality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands are equal.
   */
  friend PKGSTATE_API bool operator==(const typed_digest& lhs,
                                      const typed_digest& rhs) noexcept
  {
    return lhs.value_ == rhs.value_;
  }

  /*!
   * \brief Compare values in this exact identity domain for inequality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands differ.
   */
  friend PKGSTATE_API bool operator!=(const typed_digest& lhs,
                                      const typed_digest& rhs) noexcept
  {
    return !(lhs == rhs);
  }

  /*!
   * \brief Order values in this exact identity domain canonically.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the left operand precedes the right operand.
   */
  friend PKGSTATE_API bool operator<(const typed_digest& lhs,
                                     const typed_digest& rhs) noexcept
  {
    return lhs.value_ < rhs.value_;
  }

private:
  explicit typed_digest(digest_value value) : value_(std::move(value)) {}

  digest_value value_;
};

/*!
 * \brief Strong wrapper for an identity issued by another semantic owner.
 * \tparam Domain Empty tag naming one foreign-reference domain.
 *
 * A referenced digest preserves exact foreign identity text without claiming
 * that libpkgstate computed or owns the referenced semantic object.
 */
template<typename Domain>
class PKGSTATE_API referenced_digest final {
public:
  /*!
   * \brief Construct this reference domain from exact SHA-256 bytes.
   * \param bytes Exact SHA-256 result bytes.
   * \return Typed digest containing the supplied SHA-256 bytes.
   */
  [[nodiscard]] static referenced_digest from_sha256(sha256_digest_bytes bytes)
  {
    return referenced_digest(digest_value::from_sha256(std::move(bytes)));
  }

  /*!
   * \brief Parse canonical text into this exact reference domain.
   * \param input Canonical textual representation to parse.
   * \return Parsed canonical digest value.
   */
  [[nodiscard]] static referenced_digest parse(std::string_view input)
  {
    return referenced_digest(digest_value::parse(input));
  }

  /*!
   * \brief Return the represented wire-format version.
   * \return The represented wire-format version.
   */
  [[nodiscard]] std::uint16_t representation_version() const noexcept
  {
    return value_.representation_version();
  }

  /*!
   * \brief Return the represented digest algorithm.
   * \return The represented digest algorithm.
   */
  [[nodiscard]] digest_algorithm algorithm() const noexcept
  {
    return value_.algorithm();
  }

  /*!
   * \brief Return exact digest result bytes.
   * \return Exact digest result bytes.
   */
  [[nodiscard]] const digest_bytes& bytes() const noexcept
  {
    return value_.bytes();
  }

  /*!
   * \brief Return canonical algorithm-qualified text.
   * \return Canonical algorithm-qualified text.
   */
  [[nodiscard]] std::string string() const { return value_.string(); }

  /*!
   * \brief Compare values in this exact reference domain for equality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands are equal.
   */
  friend PKGSTATE_API bool operator==(const referenced_digest& lhs,
                                      const referenced_digest& rhs) noexcept
  {
    return lhs.value_ == rhs.value_;
  }

  /*!
   * \brief Compare values in this exact reference domain for inequality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands differ.
   */
  friend PKGSTATE_API bool operator!=(const referenced_digest& lhs,
                                      const referenced_digest& rhs) noexcept
  {
    return !(lhs == rhs);
  }

  /*!
   * \brief Order values in this exact reference domain canonically.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the left operand precedes the right operand.
   */
  friend PKGSTATE_API bool operator<(const referenced_digest& lhs,
                                     const referenced_digest& rhs) noexcept
  {
    return lhs.value_ < rhs.value_;
  }

private:
  explicit referenced_digest(digest_value value) : value_(std::move(value)) {}

  digest_value value_;
};

/*! \brief Domain tag for package_source_record_identity. */
struct package_source_record_identity_domain final {
  /*! \brief Canonical package-source-record identity protocol separator. */
  static constexpr std::string_view canonical_domain =
      "pkgstate/package-source-record/1";
};

/*! \brief Domain tag for installed_control_identity. */
struct installed_control_identity_domain final {
  /*! \brief Canonical installed-control identity protocol separator. */
  static constexpr std::string_view canonical_domain =
      "pkgstate/installed-control/3";
};

/*! \brief Domain tag for installation_receipt_identity. */
struct installation_receipt_identity_domain final {
  /*! \brief Canonical installation-receipt identity protocol separator. */
  static constexpr std::string_view canonical_domain =
      "pkgstate/installation-receipt/2";
};

/*! \brief Domain tag for installed_package_identity. */
struct installed_package_identity_domain final {
  /*! \brief Canonical installed-package identity protocol separator. */
  static constexpr std::string_view canonical_domain =
      "pkgstate/installed-package/3";
};

/*! \brief Domain tag for ownership_inventory_identity. */
struct ownership_inventory_identity_domain final {
  /*! \brief Canonical ownership-inventory identity protocol separator. */
  static constexpr std::string_view canonical_domain =
      "pkgstate/ownership-inventory/3";
};

/*! \brief Domain tag for managed_target_identity. */
struct managed_target_identity_domain final {
  /*! \brief Canonical managed-target identity protocol separator. */
  static constexpr std::string_view canonical_domain =
      "pkgstate/managed-target/1";
};

/*! \brief Domain tag for state_store_identity. */
struct state_store_identity_domain final {
  /*! \brief Canonical state-store identity protocol separator. */
  static constexpr std::string_view canonical_domain =
      "pkgstate/state-store/1";
};

/*! \brief Domain tag for root_view_identity. */
struct root_view_identity_domain final {
  /*! \brief Canonical root-view identity protocol separator. */
  static constexpr std::string_view canonical_domain =
      "pkgstate/root-view/1";
};

/*! \brief Domain tag for state_backend_identity. */
struct state_backend_identity_domain final {
  /*! \brief Canonical state-backend identity protocol separator. */
  static constexpr std::string_view canonical_domain =
      "pkgstate/state-backend/1";
};

/*! \brief Domain tag for publication_domain_identity. */
struct publication_domain_identity_domain final {
  /*! \brief Canonical publication-domain identity protocol separator. */
  static constexpr std::string_view canonical_domain =
      "pkgstate/publication-domain/1";
};

/*! \brief Domain tag for state_target_binding_identity. */
struct state_target_binding_identity_domain final {
  /*! \brief Canonical target-binding identity protocol separator. */
  static constexpr std::string_view canonical_domain =
      "pkgstate/target-binding/1";
};

/*! \brief Domain tag for installed_state_snapshot_identity. */
struct installed_state_snapshot_identity_domain final {
  /*! \brief Canonical installed-snapshot identity protocol separator. */
  static constexpr std::string_view canonical_domain =
      "pkgstate/installed-snapshot/3";
};

/*! \brief Domain tag for state_publication_request_identity. */
struct state_publication_request_identity_domain final {
  /*! \brief Canonical publication-request identity protocol separator. */
  static constexpr std::string_view canonical_domain =
      "pkgstate/publication-request/3";
};

/*! \brief Domain tag for state_publication_receipt_identity. */
struct state_publication_receipt_identity_domain final {
  /*! \brief Canonical publication-receipt identity protocol separator. */
  static constexpr std::string_view canonical_domain =
      "pkgstate/publication-receipt/3";
};

/*! \brief Domain tag for package_release_identity. */
struct package_release_reference_domain final {};
/*! \brief Domain tag for source_profile_identity. */
struct source_profile_reference_domain final {};
/*! \brief Domain tag for source_recipe_identity. */
struct source_recipe_reference_domain final {};
/*! \brief Domain tag for source_snapshot_identity. */
struct source_snapshot_reference_domain final {};
/*! \brief Domain tag for build_request_identity. */
struct build_request_reference_domain final {};
/*! \brief Domain tag for source_material_set_identity. */
struct source_material_set_reference_domain final {};
/*! \brief Domain tag for build_input_set_identity. */
struct build_input_set_reference_domain final {};
/*! \brief Domain tag for environment_policy_identity. */
struct environment_policy_reference_domain final {};
/*! \brief Domain tag for build_policy_identity. */
struct build_policy_reference_domain final {};
/*! \brief Domain tag for build_result_identity. */
struct build_result_reference_domain final {};
/*! \brief Domain tag for payload_manifest_identity. */
struct payload_manifest_reference_domain final {};
/*! \brief Domain tag for build_artifact_identity. */
struct build_artifact_reference_domain final {};
/*! \brief Domain tag for artifact_content_identity. */
struct artifact_content_reference_domain final {};
/*! \brief Domain tag for artifact_binding_identity. */
struct artifact_binding_reference_domain final {};
/*! \brief Domain tag for execution_evidence_identity. */
struct execution_evidence_reference_domain final {};
/*! \brief Domain tag for artifact_image_identity. */
struct artifact_image_reference_domain final {};
/*! \brief Domain tag for artifact_inspection_identity. */
struct artifact_inspection_reference_domain final {};
/*! \brief Domain tag for installed_regular_content_identity. */
struct installed_regular_content_reference_domain final {};
/*! \brief Domain tag for operation_plan_identity. */
struct operation_plan_reference_domain final {};
/*! \brief Domain tag for application_evidence_identity. */
struct application_evidence_reference_domain final {};
/*! \brief Domain tag for transaction_evidence_identity. */
struct transaction_evidence_reference_domain final {};
/*! \brief Domain tag for rejected_object_identity. */
struct rejected_object_reference_domain final {};
/*! \brief Domain tag for state_publication_evidence_identity. */
struct state_publication_evidence_reference_domain final {};

} // namespace detail

/*! \brief Identity of one durable package-source record. */
using package_source_record_identity =
    detail::typed_digest<detail::package_source_record_identity_domain>;
/*! \brief Identity of one complete installed-control record. */
using installed_control_identity =
    detail::typed_digest<detail::installed_control_identity_domain>;
/*! \brief Identity of one installation receipt. */
using installation_receipt_identity =
    detail::typed_digest<detail::installation_receipt_identity_domain>;
/*! \brief Identity of one complete installed package. */
using installed_package_identity =
    detail::typed_digest<detail::installed_package_identity_domain>;
/*! \brief Identity of one normalized path-to-owner inventory. */
using ownership_inventory_identity =
    detail::typed_digest<detail::ownership_inventory_identity_domain>;
/*! \brief Identity of one managed target. */
using managed_target_identity =
    detail::typed_digest<detail::managed_target_identity_domain>;
/*! \brief Identity of one durable state store. */
using state_store_identity =
    detail::typed_digest<detail::state_store_identity_domain>;
/*! \brief Identity of one target-root view. */
using root_view_identity =
    detail::typed_digest<detail::root_view_identity_domain>;
/*! \brief Identity of one state backend implementation and schema. */
using state_backend_identity =
    detail::typed_digest<detail::state_backend_identity_domain>;
/*! \brief Identity of one publication serialization domain. */
using publication_domain_identity =
    detail::typed_digest<detail::publication_domain_identity_domain>;
/*! \brief Identity binding all target-state authority components. */
using state_target_binding_identity =
    detail::typed_digest<detail::state_target_binding_identity_domain>;
/*! \brief Identity of one immutable installed-state snapshot. */
using installed_state_snapshot_identity =
    detail::typed_digest<detail::installed_state_snapshot_identity_domain>;
/*! \brief Identity of one immutable state-publication request. */
using state_publication_request_identity =
    detail::typed_digest<detail::state_publication_request_identity_domain>;
/*! \brief Identity of one immutable state-publication receipt. */
using state_publication_receipt_identity =
    detail::typed_digest<detail::state_publication_receipt_identity_domain>;

/*! \brief Foreign identity of one source-owned package release. */
using package_release_identity =
    detail::referenced_digest<detail::package_release_reference_domain>;
/*! \brief Foreign identity of one source-owned selected profile. */
using source_profile_identity =
    detail::referenced_digest<detail::source_profile_reference_domain>;
/*! \brief Foreign identity of one sealed source recipe. */
using source_recipe_identity =
    detail::referenced_digest<detail::source_recipe_reference_domain>;
/*! \brief Foreign identity of one complete source snapshot. */
using source_snapshot_identity =
    detail::referenced_digest<detail::source_snapshot_reference_domain>;
/*! \brief Foreign identity of one immutable build request. */
using build_request_identity =
    detail::referenced_digest<detail::build_request_reference_domain>;
/*! \brief Foreign identity of one admitted source-material set. */
using source_material_set_identity =
    detail::referenced_digest<detail::source_material_set_reference_domain>;
/*! \brief Foreign identity of one exact build-input set. */
using build_input_set_identity =
    detail::referenced_digest<detail::build_input_set_reference_domain>;
/*! \brief Foreign identity of one build-environment policy. */
using environment_policy_identity =
    detail::referenced_digest<detail::environment_policy_reference_domain>;
/*! \brief Foreign identity of one build policy. */
using build_policy_identity =
    detail::referenced_digest<detail::build_policy_reference_domain>;
/*! \brief Foreign identity of one build result. */
using build_result_identity =
    detail::referenced_digest<detail::build_result_reference_domain>;
/*! \brief Foreign identity of one build payload manifest. */
using payload_manifest_identity =
    detail::referenced_digest<detail::payload_manifest_reference_domain>;
/*! \brief Foreign identity of one exact build artifact. */
using build_artifact_identity =
    detail::referenced_digest<detail::build_artifact_reference_domain>;
/*! \brief Foreign identity of exact artifact content bytes. */
using artifact_content_identity =
    detail::referenced_digest<detail::artifact_content_reference_domain>;
/*! \brief Foreign identity binding artifact authority to its bytes. */
using artifact_binding_identity =
    detail::referenced_digest<detail::artifact_binding_reference_domain>;
/*! \brief Foreign identity of build-execution evidence. */
using execution_evidence_identity =
    detail::referenced_digest<detail::execution_evidence_reference_domain>;
/*! \brief Foreign identity of a normalized package image. */
using artifact_image_identity =
    detail::referenced_digest<detail::artifact_image_reference_domain>;
/*! \brief Foreign identity of package-image inspection evidence. */
using artifact_inspection_identity =
    detail::referenced_digest<detail::artifact_inspection_reference_domain>;
/*! \brief Foreign identity of decoded regular-file content. */
using installed_regular_content_identity =
    detail::referenced_digest<
        detail::installed_regular_content_reference_domain>;
/*! \brief Foreign identity of one complete operation plan. */
using operation_plan_identity =
    detail::referenced_digest<detail::operation_plan_reference_domain>;
/*! \brief Foreign identity of completed application evidence. */
using application_evidence_identity =
    detail::referenced_digest<detail::application_evidence_reference_domain>;
/*! \brief Foreign identity of orchestration-owned transaction evidence. */
using transaction_evidence_identity =
    detail::referenced_digest<detail::transaction_evidence_reference_domain>;
/*! \brief Foreign identity of one rejected target object. */
using rejected_object_identity =
    detail::referenced_digest<detail::rejected_object_reference_domain>;
/*! \brief Foreign identity of one storage-provider publication evidence item. */
using state_publication_evidence_identity =
    detail::referenced_digest<
        detail::state_publication_evidence_reference_domain>;

} // namespace pkgstate
