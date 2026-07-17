/*
 * Copyright (C) 2026 Alexandr Savca
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*!
 * \file write_transaction.h
 * \brief Backend-neutral installed-state write transactions.
 * \copyright See COPYING for license terms and COPYRIGHT for notices.
 */

#pragma once

#include <string_view>

#include <libpkgstate/error.h>
#include <libpkgstate/snapshot.h>

namespace pkgstate {

/*!
 * \brief Isolated mutation of one durable package-state store.
 *
 * A write_transaction changes only package-state storage.  It does not
 * coordinate filesystem installation or removal and is therefore not a full
 * package transaction.  Uncommitted changes are discarded on destruction.
 */
class write_transaction {
public:
  /*!
   * \brief Destroy the transaction and discard uncommitted changes.
   */
  virtual ~write_transaction() = default;

  /*!
   * \brief Return the transaction's current uncommitted state.
   */
  [[nodiscard]] virtual snapshot current() const = 0;

  /*!
   * \brief Add or replace an installed package record by package name.
   */
  virtual void put(installed_package package) = 0;

  /*!
   * \brief Remove an installed package record by name.
   * \return true when a record was removed, otherwise false.
   * \throws identity_error when \p package_name is not line-safe.
   */
  virtual bool erase(std::string_view package_name) = 0;

  /*!
   * \brief Atomically publish the transaction's current state.
   * \throws transaction_error when the transaction is already committed.
   * \throws store_error when durable publication fails.
   */
  virtual void commit() = 0;

  /*!
   * \brief Return whether commit completed successfully.
   */
  [[nodiscard]] virtual bool committed() const noexcept = 0;
};

} // namespace pkgstate
