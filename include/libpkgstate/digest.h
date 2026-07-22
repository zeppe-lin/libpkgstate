// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file digest.h
 * \brief Strongly typed installed-state identities and digest representations.
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

/*! \brief Version of the public algorithm-qualified digest representation. */
inline constexpr std::uint16_t digest_representation_version = 1;

/*! \brief Algorithm identifier carried by installed-state digest values. */
enum class digest_algorithm : std::uint16_t {
  sha256 = 1, //!< SHA-256 with a 32-byte result.
};

/*! \brief Number of bytes in a SHA-256 result. */
inline constexpr std::size_t sha256_digest_size = 32;

/*! \brief Algorithm-neutral storage for one digest result. */
using digest_bytes = std::vector<std::uint8_t>;

/*! \brief Fixed-size SHA-256 result accepted by identity factories. */
using sha256_digest_bytes = std::array<std::uint8_t, sha256_digest_size>;

/*! \brief Structured reason that a canonical digest representation is invalid. */
enum class digest_error_code {
  invalid_format,        //!< The representation does not have three fields.
  unsupported_version,  //!< The representation version is not supported.
  unsupported_algorithm, //!< The digest algorithm name is not supported.
  invalid_length,        //!< The hexadecimal digest has the wrong length.
  invalid_hex,           //!< The digest is not lowercase hexadecimal.
};

/*! \brief Reports an invalid algorithm-qualified digest representation. */
class digest_error final : public identity_error {
public:
  /*! \brief Construct a typed digest error. */
  digest_error(digest_error_code code, std::string message);

  /*! \brief Return the machine-readable failure class. */
  [[nodiscard]] digest_error_code code() const noexcept;

private:
  digest_error_code code_;
};

namespace detail {

/*! \brief Shared representation used by strongly typed identity domains. */
class digest_value final {
public:
  /*! \brief Construct a SHA-256 value from exact digest bytes. */
  [[nodiscard]] static digest_value from_sha256(sha256_digest_bytes bytes);

  /*! \brief Parse `v1:sha256:<lowercase-hex>`. */
  [[nodiscard]] static digest_value parse(std::string_view input);

  /*! \brief Return the representation version. */
  [[nodiscard]] std::uint16_t representation_version() const noexcept;

  /*! \brief Return the represented algorithm. */
  [[nodiscard]] digest_algorithm algorithm() const noexcept;

  /*! \brief Return the exact digest bytes. */
  [[nodiscard]] const digest_bytes& bytes() const noexcept;

  /*! \brief Return canonical algorithm-qualified text. */
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

/*! \brief Strong type wrapper for one installed-state identity domain. */
template<typename Domain>
class typed_digest final {
public:
  /*! \brief Construct a typed SHA-256 value from exact digest bytes. */
  [[nodiscard]] static typed_digest from_sha256(sha256_digest_bytes bytes)
  {
    return typed_digest(digest_value::from_sha256(std::move(bytes)));
  }

  /*! \brief Parse `v1:sha256:<lowercase-hex>`. */
  [[nodiscard]] static typed_digest parse(std::string_view input)
  {
    return typed_digest(digest_value::parse(input));
  }

  /*! \brief Return the canonical record domain assigned to this identity. */
  [[nodiscard]] static constexpr std::string_view canonical_domain() noexcept
  {
    return Domain::canonical_domain;
  }

  /*! \brief Return the representation version. */
  [[nodiscard]] std::uint16_t representation_version() const noexcept
  {
    return value_.representation_version();
  }

  /*! \brief Return the represented algorithm. */
  [[nodiscard]] digest_algorithm algorithm() const noexcept
  {
    return value_.algorithm();
  }

  /*! \brief Return the exact digest bytes. */
  [[nodiscard]] const digest_bytes& bytes() const noexcept
  {
    return value_.bytes();
  }

  /*! \brief Return canonical algorithm-qualified text. */
  [[nodiscard]] std::string string() const
  {
    return value_.string();
  }

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
  explicit typed_digest(digest_value value)
      : value_(std::move(value))
  {
  }

  digest_value value_;
};

struct package_release_identity_domain final {
  static constexpr std::string_view canonical_domain =
      "pkgstate/package-release/1";
};
struct installed_control_identity_domain final {
  static constexpr std::string_view canonical_domain =
      "pkgstate/installed-control/1";
};
struct installed_package_identity_domain final {
  static constexpr std::string_view canonical_domain =
      "pkgstate/installed-package/1";
};
struct ownership_inventory_identity_domain final {
  static constexpr std::string_view canonical_domain =
      "pkgstate/ownership-inventory/1";
};
struct managed_target_identity_domain final {
  static constexpr std::string_view canonical_domain =
      "pkgstate/managed-target/1";
};
struct state_store_identity_domain final {
  static constexpr std::string_view canonical_domain =
      "pkgstate/state-store/1";
};
struct root_view_identity_domain final {
  static constexpr std::string_view canonical_domain =
      "pkgstate/root-view/1";
};
struct state_backend_identity_domain final {
  static constexpr std::string_view canonical_domain =
      "pkgstate/state-backend/1";
};
struct publication_domain_identity_domain final {
  static constexpr std::string_view canonical_domain =
      "pkgstate/publication-domain/1";
};
struct state_target_binding_identity_domain final {
  static constexpr std::string_view canonical_domain =
      "pkgstate/target-binding/1";
};
struct installed_state_snapshot_identity_domain final {
  static constexpr std::string_view canonical_domain =
      "pkgstate/installed-snapshot/1";
};
struct state_publication_request_identity_domain final {
  static constexpr std::string_view canonical_domain =
      "pkgstate/publication-request/1";
};
struct state_publication_receipt_identity_domain final {
  static constexpr std::string_view canonical_domain =
      "pkgstate/publication-receipt/1";
};

} // namespace detail

/*! \brief Identity of one package release independent of artifact transport. */
using package_release_identity =
    detail::typed_digest<detail::package_release_identity_domain>;

/*! \brief Identity of durable historical installed control. */
using installed_control_identity =
    detail::typed_digest<detail::installed_control_identity_domain>;

/*! \brief Identity of one canonical installed package record. */
using installed_package_identity =
    detail::typed_digest<detail::installed_package_identity_domain>;

/*! \brief Identity of the admitted path-to-owner relation. */
using ownership_inventory_identity =
    detail::typed_digest<detail::ownership_inventory_identity_domain>;

/*! \brief Identity of one managed package target. */
using managed_target_identity =
    detail::typed_digest<detail::managed_target_identity_domain>;

/*! \brief Identity of one durable installed-state store. */
using state_store_identity =
    detail::typed_digest<detail::state_store_identity_domain>;

/*! \brief Identity of one logical target root view. */
using root_view_identity =
    detail::typed_digest<detail::root_view_identity_domain>;

/*! \brief Identity of one installed-state storage backend. */
using state_backend_identity =
    detail::typed_digest<detail::state_backend_identity_domain>;

/*! \brief Identity of one state-publication and locking domain. */
using publication_domain_identity =
    detail::typed_digest<detail::publication_domain_identity_domain>;

/*! \brief Identity of one durable state-target binding. */
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

} // namespace pkgstate
