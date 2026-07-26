// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file adapter.h
 *  \brief Admission of native libpkgbuild authority into installed state.
 */
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include <libpkgbuild/result.h>
#include <libpkgimage/inspection_receipt.h>
#include <libpkgstate/installed_control.h>
#include <libpkgstate/package_source_record.h>

namespace pkgstate::build_adapter {

enum class projection_error_code : std::uint8_t {
  build_result = 1,
  source_binding = 2,
  artifact_binding = 3,
  payload_mismatch = 4,
  identity_translation = 5,
};

class projection_error final : public std::invalid_argument {
public:
  projection_error(projection_error_code code, std::string message);
  [[nodiscard]] projection_error_code code() const noexcept;
private:
  projection_error_code code_;
};


/*! \brief Source-bound build authority admitted by this adapter. */
class build_authority final {
public:
  [[nodiscard]] const package_source_record& source() const noexcept;
  [[nodiscard]] const build_provenance& provenance() const noexcept;
private:
  build_authority(package_source_record source,
                  build_provenance provenance);
  package_source_record source_;
  build_provenance provenance_;
  friend build_authority project_build(
      const package_source_record& source,
      const pkgbuild::build_result& build,
      const pkgimage::inspected_package_image& image);
};

/*! \brief Retain one verified successful build as native state provenance.
 *
 * The source record must be the exact libpkgstate-source projection of the
 * build request's sealed source snapshot. The inspected image must name the
 * exact artifact bytes and match the complete ordered build payload. No
 * planner, application, filesystem, or store authority is created here.
 */
[[nodiscard]] build_authority project_build(
    const package_source_record& source,
    const pkgbuild::build_result& build,
    const pkgimage::inspected_package_image& image);

} // namespace pkgstate::build_adapter
