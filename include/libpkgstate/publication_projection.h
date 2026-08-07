// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file publication_projection.h
 *  \brief Pure projection of one exact publication request.
 */
#pragma once

#include <libpkgstate/export.h>
#include <libpkgstate/publication_request.h>
#include <libpkgstate/snapshot.h>

namespace pkgstate {

/*! \brief Apply one validated publication request to its exact prior snapshot.
 *
 * The function performs no storage access or target mutation. It rejects a
 * foreign target, a stale prior snapshot, and any delta that contradicts the
 * supplied prior state. On success it returns the exact resulting snapshot.
 */
[[nodiscard]] PKGSTATE_API snapshot project_publication_request(
    const state_publication_request& request,
    const snapshot& actual_prior);

} // namespace pkgstate
