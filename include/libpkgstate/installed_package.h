// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file installed_package.h
 *  \brief Canonical installed package records.
 */
#pragma once

#include <cstddef>

#include <libpkgstate/digest.h>
#include <libpkgstate/installation_receipt.h>

namespace pkgstate {

class installed_package final {
public:
  [[nodiscard]] static installed_package make(installation_receipt receipt);
  [[nodiscard]] const installed_package_identity& identity() const noexcept;
  [[nodiscard]] const installation_receipt& receipt() const noexcept;
  [[nodiscard]] const package_release& release() const noexcept;
  [[nodiscard]] const installed_control& control() const noexcept;
  [[nodiscard]] const state_target_binding& target_binding() const noexcept;
  [[nodiscard]] const std::vector<owned_entry>& manifest() const noexcept;
  [[nodiscard]] std::size_t size() const noexcept;
  [[nodiscard]] const owned_entry* find(const package_path& path) const noexcept;
  [[nodiscard]] bool owns(const package_path& path) const noexcept;
  friend bool operator==(const installed_package& lhs,
                         const installed_package& rhs) noexcept;
  friend bool operator!=(const installed_package& lhs,
                         const installed_package& rhs) noexcept;
  friend bool operator<(const installed_package& lhs,
                        const installed_package& rhs) noexcept;
private:
  installed_package(installed_package_identity identity,
                    installation_receipt receipt);
  installed_package_identity identity_;
  installation_receipt receipt_;
};

} // namespace pkgstate
