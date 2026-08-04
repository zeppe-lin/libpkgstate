// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file installed_package.h
 * \brief Canonical installed package records.
 */
#pragma once

#include <libpkgstate/export.h>

#include <cstddef>

#include <libpkgstate/digest.h>
#include <libpkgstate/installation_receipt.h>

namespace pkgstate {

/*!
 * \brief Identified installed package authority.
 *
 * An installed package is the durable package-level projection of one complete
 * installation receipt. It retains the receipt without flattening source,
 * build, target, ownership, planning, application, or transaction authority.
 */
class PKGSTATE_API installed_package final {
public:
  /*!
   * \brief Identify one complete installation receipt as an installed package.
   * \param receipt Complete admitted installation authority.
   * \return Immutable installed package identified from \p receipt.
   */
  [[nodiscard]] static installed_package make(installation_receipt receipt);

  /*!
   * \brief Return the canonical installed-package identity.
   * \return The canonical installed-package identity.
   */
  [[nodiscard]] const installed_package_identity& identity() const noexcept;
  /*!
   * \brief Return the complete issuing installation receipt.
   * \return The complete issuing installation receipt.
   */
  [[nodiscard]] const installation_receipt& receipt() const noexcept;
  /*!
   * \brief Return source-authoritative package release coordinates.
   * \return Source-authoritative package release coordinates.
   */
  [[nodiscard]] const package_release& release() const noexcept;
  /*!
   * \brief Return complete installed source and build control.
   * \return Complete installed source and build control.
   */
  [[nodiscard]] const installed_control& control() const noexcept;
  /*!
   * \brief Return the exact managed target-state binding.
   * \return The exact managed target-state binding.
   */
  [[nodiscard]] const state_target_binding& target_binding() const noexcept;
  /*!
   * \brief Return the canonical installed object manifest.
   * \return The canonical installed object manifest.
   */
  [[nodiscard]] const std::vector<owned_entry>& manifest() const noexcept;
  /*!
   * \brief Return the number of owned manifest entries.
   * \return The number of owned manifest entries.
   */
  [[nodiscard]] std::size_t size() const noexcept;

  /*!
   * \brief Find one exact owned path.
   * \param path Canonical absolute package path.
   * \return Pointer valid for this package's lifetime, or `nullptr` when the
   * path is not owned.
   */
  [[nodiscard]] const owned_entry* find(const package_path& path) const noexcept;

  /*!
   * \brief Test whether this package owns one exact path.
   * \param path Canonical absolute package path.
   * \return `true` exactly when find() returns an entry.
   */
  [[nodiscard]] bool owns(const package_path& path) const noexcept;

  /*!
   * \brief Compare complete installed packages for equality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands are equal.
   */
  friend PKGSTATE_API bool operator==(const installed_package& lhs,
                                      const installed_package& rhs) noexcept;
  /*!
   * \brief Compare complete installed packages for inequality.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the operands differ.
   */
  friend PKGSTATE_API bool operator!=(const installed_package& lhs,
                                      const installed_package& rhs) noexcept;
  /*!
   * \brief Order installed packages by canonical package reference.
   * \param lhs Left operand.
   * \param rhs Right operand.
   * \return Whether the left operand precedes the right operand.
   */
  friend PKGSTATE_API bool operator<(const installed_package& lhs,
                                     const installed_package& rhs) noexcept;

private:
  installed_package(installed_package_identity identity,
                    installation_receipt receipt);

  installed_package_identity identity_;
  installation_receipt receipt_;
};

} // namespace pkgstate
