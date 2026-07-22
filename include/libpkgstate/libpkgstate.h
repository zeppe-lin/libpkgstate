// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file libpkgstate.h
 * \brief Umbrella header for the libpkgstate public API.
 */

#pragma once

#include <libpkgstate/error.h>
#include <libpkgstate/digest.h>
#include <libpkgstate/package_release.h>
#include <libpkgstate/installed_control.h>
#include <libpkgstate/state_target_binding.h>
#include <libpkgstate/installed_package.h>
#include <libpkgstate/legacy_installed_package.h>
#include <libpkgstate/owned_entry.h>
#include <libpkgstate/package_identity.h>
#include <libpkgstate/package_path.h>
#include <libpkgstate/snapshot.h>
#include <libpkgstate/publication_request.h>
#include <libpkgstate/publication_receipt.h>
#include <libpkgstate/canonical_store.h>
#include <libpkgstate/canonical_generation_store.h>
#include <libpkgstate/legacy_completeness.h>
#include <libpkgstate/legacy_snapshot.h>

#include <libpkgstate/write_transaction.h>
#include <libpkgstate/store.h>
#include <libpkgstate/legacy_text_store.h>
