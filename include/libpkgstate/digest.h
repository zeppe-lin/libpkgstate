// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file digest.h
 *  \brief Strongly typed native installed-state identities.
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <libpkgstate/error.h>

namespace pkgstate {

inline constexpr std::uint16_t digest_representation_version = 1;
enum class digest_algorithm : std::uint16_t { sha256 = 1 };
inline constexpr std::size_t sha256_digest_size = 32;
using digest_bytes = std::vector<std::uint8_t>;
using sha256_digest_bytes = std::array<std::uint8_t, sha256_digest_size>;

enum class digest_error_code {
  invalid_format,
  unsupported_version,
  unsupported_algorithm,
  invalid_length,
  invalid_hex,
};

class digest_error final : public identity_error {
public:
  digest_error(digest_error_code code, std::string message);
  [[nodiscard]] digest_error_code code() const noexcept;
private:
  digest_error_code code_;
};

namespace detail {

class digest_value final {
public:
  [[nodiscard]] static digest_value from_sha256(sha256_digest_bytes bytes);
  [[nodiscard]] static digest_value parse(std::string_view input);
  [[nodiscard]] std::uint16_t representation_version() const noexcept;
  [[nodiscard]] digest_algorithm algorithm() const noexcept;
  [[nodiscard]] const digest_bytes& bytes() const noexcept;
  [[nodiscard]] std::string string() const;
  friend bool operator==(const digest_value& lhs,
                         const digest_value& rhs) noexcept;
  friend bool operator!=(const digest_value& lhs,
                         const digest_value& rhs) noexcept;
  friend bool operator<(const digest_value& lhs,
                        const digest_value& rhs) noexcept;
private:
  digest_value(std::uint16_t representation_version,
               digest_algorithm algorithm,
               digest_bytes bytes);
  std::uint16_t representation_version_;
  digest_algorithm algorithm_;
  digest_bytes bytes_;
};

template<typename Domain>
class typed_digest final {
public:
  [[nodiscard]] static typed_digest from_sha256(sha256_digest_bytes bytes)
  {
    return typed_digest(digest_value::from_sha256(std::move(bytes)));
  }
  [[nodiscard]] static typed_digest parse(std::string_view input)
  {
    return typed_digest(digest_value::parse(input));
  }
  [[nodiscard]] static constexpr std::string_view canonical_domain() noexcept
  {
    return Domain::canonical_domain;
  }
  [[nodiscard]] std::uint16_t representation_version() const noexcept
  {
    return value_.representation_version();
  }
  [[nodiscard]] digest_algorithm algorithm() const noexcept
  {
    return value_.algorithm();
  }
  [[nodiscard]] const digest_bytes& bytes() const noexcept
  {
    return value_.bytes();
  }
  [[nodiscard]] std::string string() const { return value_.string(); }
  friend bool operator==(const typed_digest& lhs,
                         const typed_digest& rhs) noexcept
  {
    return lhs.value_ == rhs.value_;
  }
  friend bool operator!=(const typed_digest& lhs,
                         const typed_digest& rhs) noexcept
  {
    return !(lhs == rhs);
  }
  friend bool operator<(const typed_digest& lhs,
                        const typed_digest& rhs) noexcept
  {
    return lhs.value_ < rhs.value_;
  }
private:
  explicit typed_digest(digest_value value) : value_(std::move(value)) {}
  digest_value value_;
};

template<typename Domain>
class referenced_digest final {
public:
  [[nodiscard]] static referenced_digest from_sha256(sha256_digest_bytes bytes)
  {
    return referenced_digest(digest_value::from_sha256(std::move(bytes)));
  }
  [[nodiscard]] static referenced_digest parse(std::string_view input)
  {
    return referenced_digest(digest_value::parse(input));
  }
  [[nodiscard]] std::uint16_t representation_version() const noexcept
  {
    return value_.representation_version();
  }
  [[nodiscard]] digest_algorithm algorithm() const noexcept
  {
    return value_.algorithm();
  }
  [[nodiscard]] const digest_bytes& bytes() const noexcept
  {
    return value_.bytes();
  }
  [[nodiscard]] std::string string() const { return value_.string(); }
  friend bool operator==(const referenced_digest& lhs,
                         const referenced_digest& rhs) noexcept
  {
    return lhs.value_ == rhs.value_;
  }
  friend bool operator!=(const referenced_digest& lhs,
                         const referenced_digest& rhs) noexcept
  {
    return !(lhs == rhs);
  }
  friend bool operator<(const referenced_digest& lhs,
                        const referenced_digest& rhs) noexcept
  {
    return lhs.value_ < rhs.value_;
  }
private:
  explicit referenced_digest(digest_value value) : value_(std::move(value)) {}
  digest_value value_;
};

#define PKGSTATE_STATE_DOMAIN(name, text)                                      \
  struct name##_domain final {                                                 \
    static constexpr std::string_view canonical_domain = text;                 \
  }

PKGSTATE_STATE_DOMAIN(package_source_record_identity,
                      "pkgstate/package-source-record/1");
PKGSTATE_STATE_DOMAIN(installed_control_identity,
                      "pkgstate/installed-control/3");
PKGSTATE_STATE_DOMAIN(installation_receipt_identity,
                      "pkgstate/installation-receipt/2");
PKGSTATE_STATE_DOMAIN(installed_package_identity,
                      "pkgstate/installed-package/3");
PKGSTATE_STATE_DOMAIN(ownership_inventory_identity,
                      "pkgstate/ownership-inventory/3");
PKGSTATE_STATE_DOMAIN(managed_target_identity, "pkgstate/managed-target/1");
PKGSTATE_STATE_DOMAIN(state_store_identity, "pkgstate/state-store/1");
PKGSTATE_STATE_DOMAIN(root_view_identity, "pkgstate/root-view/1");
PKGSTATE_STATE_DOMAIN(state_backend_identity, "pkgstate/state-backend/1");
PKGSTATE_STATE_DOMAIN(publication_domain_identity,
                      "pkgstate/publication-domain/1");
PKGSTATE_STATE_DOMAIN(state_target_binding_identity,
                      "pkgstate/target-binding/1");
PKGSTATE_STATE_DOMAIN(installed_state_snapshot_identity,
                      "pkgstate/installed-snapshot/3");
PKGSTATE_STATE_DOMAIN(state_publication_request_identity,
                      "pkgstate/publication-request/3");
PKGSTATE_STATE_DOMAIN(state_publication_receipt_identity,
                      "pkgstate/publication-receipt/3");
#undef PKGSTATE_STATE_DOMAIN

struct package_release_reference_domain final {};
struct source_profile_reference_domain final {};
struct source_recipe_reference_domain final {};
struct source_snapshot_reference_domain final {};
struct build_request_reference_domain final {};
struct source_material_set_reference_domain final {};
struct build_input_set_reference_domain final {};
struct environment_policy_reference_domain final {};
struct build_policy_reference_domain final {};
struct build_result_reference_domain final {};
struct payload_manifest_reference_domain final {};
struct build_artifact_reference_domain final {};
struct artifact_content_reference_domain final {};
struct artifact_binding_reference_domain final {};
struct execution_evidence_reference_domain final {};
struct artifact_image_reference_domain final {};
struct artifact_inspection_reference_domain final {};
struct installed_regular_content_reference_domain final {};
struct operation_plan_reference_domain final {};
struct application_evidence_reference_domain final {};
struct transaction_evidence_reference_domain final {};
struct rejected_object_reference_domain final {};
struct state_publication_evidence_reference_domain final {};

} // namespace detail

using package_source_record_identity =
    detail::typed_digest<detail::package_source_record_identity_domain>;
using installed_control_identity =
    detail::typed_digest<detail::installed_control_identity_domain>;
using installation_receipt_identity =
    detail::typed_digest<detail::installation_receipt_identity_domain>;
using installed_package_identity =
    detail::typed_digest<detail::installed_package_identity_domain>;
using ownership_inventory_identity =
    detail::typed_digest<detail::ownership_inventory_identity_domain>;
using managed_target_identity =
    detail::typed_digest<detail::managed_target_identity_domain>;
using state_store_identity =
    detail::typed_digest<detail::state_store_identity_domain>;
using root_view_identity =
    detail::typed_digest<detail::root_view_identity_domain>;
using state_backend_identity =
    detail::typed_digest<detail::state_backend_identity_domain>;
using publication_domain_identity =
    detail::typed_digest<detail::publication_domain_identity_domain>;
using state_target_binding_identity =
    detail::typed_digest<detail::state_target_binding_identity_domain>;
using installed_state_snapshot_identity =
    detail::typed_digest<detail::installed_state_snapshot_identity_domain>;
using state_publication_request_identity =
    detail::typed_digest<detail::state_publication_request_identity_domain>;
using state_publication_receipt_identity =
    detail::typed_digest<detail::state_publication_receipt_identity_domain>;

using package_release_identity =
    detail::referenced_digest<detail::package_release_reference_domain>;
using source_profile_identity =
    detail::referenced_digest<detail::source_profile_reference_domain>;
using source_recipe_identity =
    detail::referenced_digest<detail::source_recipe_reference_domain>;
using source_snapshot_identity =
    detail::referenced_digest<detail::source_snapshot_reference_domain>;
using build_request_identity =
    detail::referenced_digest<detail::build_request_reference_domain>;
using source_material_set_identity =
    detail::referenced_digest<detail::source_material_set_reference_domain>;
using build_input_set_identity =
    detail::referenced_digest<detail::build_input_set_reference_domain>;
using environment_policy_identity =
    detail::referenced_digest<detail::environment_policy_reference_domain>;
using build_policy_identity =
    detail::referenced_digest<detail::build_policy_reference_domain>;
using build_result_identity =
    detail::referenced_digest<detail::build_result_reference_domain>;
using payload_manifest_identity =
    detail::referenced_digest<detail::payload_manifest_reference_domain>;
using build_artifact_identity =
    detail::referenced_digest<detail::build_artifact_reference_domain>;
using artifact_content_identity =
    detail::referenced_digest<detail::artifact_content_reference_domain>;
using artifact_binding_identity =
    detail::referenced_digest<detail::artifact_binding_reference_domain>;
using execution_evidence_identity =
    detail::referenced_digest<detail::execution_evidence_reference_domain>;
using artifact_image_identity =
    detail::referenced_digest<detail::artifact_image_reference_domain>;
using artifact_inspection_identity =
    detail::referenced_digest<detail::artifact_inspection_reference_domain>;
using installed_regular_content_identity =
    detail::referenced_digest<
        detail::installed_regular_content_reference_domain>;
using operation_plan_identity =
    detail::referenced_digest<detail::operation_plan_reference_domain>;
using application_evidence_identity =
    detail::referenced_digest<detail::application_evidence_reference_domain>;
using transaction_evidence_identity =
    detail::referenced_digest<detail::transaction_evidence_reference_domain>;
using rejected_object_identity =
    detail::referenced_digest<detail::rejected_object_reference_domain>;
using state_publication_evidence_identity =
    detail::referenced_digest<detail::state_publication_evidence_reference_domain>;

} // namespace pkgstate
