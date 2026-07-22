// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <libpkgstate/publication_request.h>
#include <libpkgstate/snapshot.h>

namespace pkgstate::detail {

[[nodiscard]] snapshot
project_publication_request(const state_publication_request& request,
                            const snapshot& actual_prior);

} // namespace pkgstate::detail
