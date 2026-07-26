// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file adapter.h
 *  \brief Durable projection of sealed libpkgsource authority into state.
 */
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include <libpkgsource/snapshot.h>
#include <libpkgstate/package_source_record.h>

namespace pkgstate::source_adapter {

enum class projection_error_code : std::uint8_t {
  identity_translation = 1,
  vocabulary_translation = 2,
  architecture_selection = 3,
  record_construction = 4,
};

class projection_error final : public std::invalid_argument {
public:
  projection_error(projection_error_code code, std::string message);
  [[nodiscard]] projection_error_code code() const noexcept;
private:
  projection_error_code code_;
};

/*! \brief Project one sealed source snapshot into durable state control.
 *
 * The caller supplies the selected build and target architecture. The adapter
 * verifies those selections against the source authority and retains the
 * package release, runtime/lifecycle control, selected build profiles, source
 * recipe identity, and source snapshot identity. It performs no resolution,
 * build, installation, or state I/O.
 */
[[nodiscard]] package_source_record project_source(
    const pkgsource::source_snapshot& source,
    const pkgsource::architecture_reference& selected_build,
    const pkgsource::architecture_reference& selected_target);

} // namespace pkgstate::source_adapter
