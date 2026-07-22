// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file write_transaction.h
 * \brief Compatibility-state write transactions.
 */

#pragma once

#include <string_view>

#include <libpkgstate/error.h>
#include <libpkgstate/legacy_installed_package.h>
#include <libpkgstate/legacy_snapshot.h>

namespace pkgstate {

/*!
 * \brief Isolated mutation of one compatibility package-state store.
 *
 * A write_transaction changes only historical compatibility storage.  It does
 * not accept complete canonical installed packages, perform expected-snapshot
 * comparison, coordinate filesystem application, or provide a complete package
 * transaction.  Uncommitted changes are discarded on destruction.
 */
class write_transaction {
public:
  /*! \brief Destroy the transaction and discard uncommitted changes. */
  virtual ~write_transaction() = default;

  /*! \brief Return the current uncommitted compatibility state. */
  [[nodiscard]] virtual legacy_snapshot current() const = 0;

  /*! \brief Add or replace a compatibility package record by package name. */
  virtual void put(legacy_installed_package package) = 0;

  /*!
   * \brief Remove a compatibility package record by name.
   * \return true when a record was removed, otherwise false.
   * \throws identity_error when \p package_name is not line-safe.
   */
  virtual bool erase(std::string_view package_name) = 0;

  /*!
   * \brief Atomically publish the current compatibility database bytes.
   * \throws transaction_error when the transaction is already committed.
   * \throws store_error when compatibility publication fails.
   */
  virtual void commit() = 0;

  /*! \brief Return whether commit completed successfully. */
  [[nodiscard]] virtual bool committed() const noexcept = 0;
};

} // namespace pkgstate
