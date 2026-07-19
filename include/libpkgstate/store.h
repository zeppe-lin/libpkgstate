// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file store.h
 * \brief Backend-neutral package-state storage interface.
 */

#pragma once

#include <memory>

#include <libpkgstate/error.h>
#include <libpkgstate/write_transaction.h>

namespace pkgstate {

/*!
 * \brief Durable storage backend for installed package state.
 */
class store {
public:
  /*!
   * \brief Destroy the store backend.
   */
  virtual ~store() = default;

  /*!
   * \brief Read one immutable state snapshot.
   * \throws store_error on locking, parsing, or I/O failure.
   */
  [[nodiscard]] virtual snapshot read() const = 0;

  /*!
   * \brief Begin an isolated exclusive state write.
   * \return Transaction initialized from the current durable snapshot.
   * \throws store_error when the store cannot be locked or read.
   */
  [[nodiscard]] virtual std::unique_ptr<write_transaction>
  begin_write() const = 0;
};

} // namespace pkgstate
