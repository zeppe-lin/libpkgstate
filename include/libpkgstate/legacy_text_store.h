// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file legacy_text_store.h
 * \brief Compatibility backend for the historical package database format.
 */

#pragma once

#include <filesystem>
#include <memory>

#include <libpkgstate/store.h>

namespace pkgstate {

/*!
 * \brief Line-oriented compatibility store for `/var/lib/pkg/db`.
 *
 * Each record contains package name, package version, zero or more owned
 * paths, and a blank record terminator.  The backend is an original
 * implementation of that documented format.  It acquires non-blocking
 * advisory locks on the database directory so it can coexist safely with
 * the tools being replaced during migration.
 */
class legacy_text_store final : public store {
public:
  /*!
   * \brief Bind the store to an explicit database file.
   * \param database Path to the line-oriented package database.
   */
  explicit legacy_text_store(std::filesystem::path database);

  /*!
   * \brief Return the configured database path.
   */
  [[nodiscard]] const std::filesystem::path&
  database_path() const noexcept;

  [[nodiscard]] snapshot read() const override;

  [[nodiscard]] std::unique_ptr<write_transaction>
  begin_write() const override;

private:
  std::filesystem::path database_;
};

} // namespace pkgstate
