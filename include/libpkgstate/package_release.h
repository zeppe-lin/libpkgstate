// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file package_release.h
 *  \brief Source-authoritative package release facts retained in state.
 */
#pragma once

#include <libpkgstate/export.h>

#include <cstdint>
#include <string>

#include <libpkgstate/digest.h>
#include <libpkgstate/model.h>

namespace pkgstate {

class PKGSTATE_API package_release final {
public:
  package_release(package_release_identity identity,
                  package_reference package,
                  std::string version,
                  std::uint32_t release);
  [[nodiscard]] const package_release_identity& identity() const noexcept;
  [[nodiscard]] const package_reference& package() const noexcept;
  [[nodiscard]] const std::string& name() const noexcept;
  [[nodiscard]] const std::string& version() const noexcept;
  [[nodiscard]] std::uint32_t release() const noexcept;
  [[nodiscard]] std::string version_release() const;
  friend PKGSTATE_API bool operator==(const package_release& lhs,
                         const package_release& rhs) noexcept;
  friend PKGSTATE_API bool operator!=(const package_release& lhs,
                         const package_release& rhs) noexcept;
  friend PKGSTATE_API bool operator<(const package_release& lhs,
                        const package_release& rhs) noexcept;
private:
  package_release_identity identity_;
  package_reference package_;
  std::string version_;
  std::uint32_t release_;
};

} // namespace pkgstate
