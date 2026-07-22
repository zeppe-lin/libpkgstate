// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file canonical_generation_store.h
 * \brief Immutable-generation canonical installed-state backend.
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

#include <libpkgstate/canonical_store.h>
#include <libpkgstate/snapshot.h>
#include <libpkgstate/state_target_binding.h>

namespace pkgstate {

/*! \brief Current canonical generation storage-format version. */
inline constexpr std::uint16_t canonical_generation_storage_version = 1;

/*! \brief Receipt-visible canonical generation storage-format identifier. */
inline constexpr std::string_view canonical_generation_storage_format =
    "libpkgstate-generation-v1";

/*!
 * \brief Canonical backend using immutable generations and atomic selection.
 *
 * The backend stores one complete canonical snapshot in each immutable
 * generation directory.  A single current selector names the authoritative
 * generation.  Reads take a non-blocking shared lock on the store directory;
 * compare-and-publish takes a non-blocking exclusive lock through the
 * canonical_store transaction hook.
 *
 * Construction creates and selects an empty generation when necessary and
 * durably binds the directory to exactly one state_target_binding. It never
 * rewrites the historical /var/lib/pkg/db compatibility database.
 */
class canonical_generation_store final : public canonical_store {
public:
  /*!
   * \brief Open or initialize one canonical generation store.
   * \param root Complete pathname of the canonical store directory.
   * \param target_binding Durable target-state binding owned by the store.
   * \throws store_error on layout, locking, binding, or durability failure.
   */
  canonical_generation_store(std::filesystem::path root,
                             state_target_binding target_binding);

  canonical_generation_store(const canonical_generation_store&) = delete;
  canonical_generation_store&
  operator=(const canonical_generation_store&) = delete;

  /*! \brief Return the configured store directory pathname. */
  [[nodiscard]] const std::filesystem::path& root_path() const noexcept;

  /*! \brief Return the durable target binding owned by this store. */
  [[nodiscard]] const state_target_binding&
  target_binding() const noexcept;

  /*! \brief Read and validate the currently selected complete snapshot. */
  [[nodiscard]] snapshot read() const override;

protected:
  /*! \brief Lock the store exclusively and reread authoritative state. */
  [[nodiscard]] std::unique_ptr<canonical_publication_transaction>
  begin_publication() const override;

private:
  void initialize();

  std::filesystem::path root_;
  state_target_binding target_binding_;
};

} // namespace pkgstate
