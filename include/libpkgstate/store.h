// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file store.h
 * \brief Compatibility package-state storage interface.
 */

#pragma once

#include <memory>

#include <libpkgstate/error.h>
#include <libpkgstate/legacy_snapshot.h>
#include <libpkgstate/write_transaction.h>

namespace pkgstate {

/*!
 * \brief Storage backend for explicitly incomplete compatibility state.
 *
 * This interface predates canonical stale-safe publication.  It reads and
 * mutates `legacy_snapshot` values only.  Canonical `snapshot` publication is
 * introduced through a separate compare-and-publish interface.
 */
class store {
public:
  /*! \brief Destroy the compatibility store backend. */
  virtual ~store() = default;

  /*!
   * \brief Read one immutable compatibility snapshot.
   * \throws store_error on locking, parsing, or I/O failure.
   */
  [[nodiscard]] virtual legacy_snapshot read() const = 0;

  /*!
   * \brief Begin an isolated exclusive compatibility-state write.
   * \return Transaction initialized from current compatibility state.
   * \throws store_error when the store cannot be locked or read.
   */
  [[nodiscard]] virtual std::unique_ptr<write_transaction>
  begin_write() const = 0;
};

} // namespace pkgstate
