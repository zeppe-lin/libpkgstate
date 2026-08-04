// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file package_release.h
 * \brief Source-authoritative package release coordinates retained in state.
 */
#pragma once

#include <libpkgstate/export.h>

#include <cstdint>
#include <string>

#include <libpkgstate/digest.h>
#include <libpkgstate/model.h>

namespace pkgstate {

/*!
 * \brief Exact package release authority imported from the source domain.
 *
 * The foreign package-release identity is retained beside the normalized
 * package name, version, and positive release number. libpkgstate validates
 * representation safety but does not recompute the source-owned identity.
 */
class PKGSTATE_API package_release final {
public:
  /*!
   * \brief Construct exact package release coordinates.
   * \param identity Source-owned identity of this package release.
   * \param package Canonical package reference.
   * \param version Non-empty printable version without a slash.
   * \param release Positive package release number.
   * \throws state_error when \p version is unsafe or \p release is zero.
   */
  package_release(package_release_identity identity,
                  package_reference package,
                  std::string version,
                  std::uint32_t release);

  /*! \brief Return the source-owned package-release identity. */
  [[nodiscard]] const package_release_identity& identity() const noexcept;
  /*! \brief Return the canonical package reference. */
  [[nodiscard]] const package_reference& package() const noexcept;
  /*! \brief Return the canonical package name. */
  [[nodiscard]] const std::string& name() const noexcept;
  /*! \brief Return the exact package version. */
  [[nodiscard]] const std::string& version() const noexcept;
  /*! \brief Return the positive package release number. */
  [[nodiscard]] std::uint32_t release() const noexcept;
  /*! \brief Return canonical `version-release` text. */
  [[nodiscard]] std::string version_release() const;

  /*! \brief Compare complete package release values for equality. */
  friend PKGSTATE_API bool operator==(const package_release& lhs,
                                      const package_release& rhs) noexcept;
  /*! \brief Compare complete package release values for inequality. */
  friend PKGSTATE_API bool operator!=(const package_release& lhs,
                                      const package_release& rhs) noexcept;
  /*! \brief Order releases by package, version, release, and identity. */
  friend PKGSTATE_API bool operator<(const package_release& lhs,
                                     const package_release& rhs) noexcept;

private:
  package_release_identity identity_;
  package_reference package_;
  std::string version_;
  std::uint32_t release_;
};

} // namespace pkgstate
