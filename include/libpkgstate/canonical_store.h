// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file canonical_store.h
 * \brief Stale-safe canonical installed-state publication interface.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <libpkgstate/legacy_import.h>
#include <libpkgstate/publication_receipt.h>
#include <libpkgstate/publication_request.h>
#include <libpkgstate/snapshot.h>

namespace pkgstate {

/*!
 * \brief Constrained result reported by one backend publication primitive.
 *
 * This value describes only what happened after canonical_store validated the
 * expected snapshot and computed the one permitted resulting snapshot.  It
 * cannot represent stale state; that outcome is owned by compare_and_publish().
 */
class state_publication_backend_result final {
public:
  /*! \brief Report confirmed publication and durability. */
  [[nodiscard]] static state_publication_backend_result
  published(
      state_storage_atomicity_boundary atomicity,
      std::vector<state_publication_evidence_identity> evidence = {});

  /*! \brief Report semantic rejection before crossing publication boundary. */
  [[nodiscard]] static state_publication_backend_result
  request_rejected(
      std::vector<state_publication_evidence_identity> evidence = {});

  /*! \brief Report failure before crossing publication boundary. */
  [[nodiscard]] static state_publication_backend_result
  failed_before_publication(
      std::vector<state_publication_evidence_identity> evidence = {});

  /*! \brief Report visible publication without durable confirmation. */
  [[nodiscard]] static state_publication_backend_result
  published_but_durability_unconfirmed(
      state_storage_atomicity_boundary atomicity,
      std::vector<state_publication_evidence_identity> evidence = {});

  /*! \brief Report an attempt requiring an authoritative reread. */
  [[nodiscard]] static state_publication_backend_result
  indeterminate(
      state_storage_atomicity_boundary atomicity,
      bool resulting_snapshot_established,
      std::vector<state_publication_evidence_identity> evidence = {});

  /*! \brief Return the semantic publication outcome. */
  [[nodiscard]] state_publication_outcome outcome() const noexcept;

  /*! \brief Return the actual state-storage atomicity boundary. */
  [[nodiscard]] state_storage_atomicity_boundary
  atomicity_boundary() const noexcept;

  /*! \brief Return whether the requested result is established. */
  [[nodiscard]] bool resulting_snapshot_established() const noexcept;

  /*! \brief Return subordinate backend evidence in identity order. */
  [[nodiscard]] const std::vector<state_publication_evidence_identity>&
  subordinate_evidence() const noexcept;

private:
  state_publication_backend_result(
      state_publication_outcome outcome,
      state_storage_atomicity_boundary atomicity,
      bool resulting_snapshot_established,
      std::vector<state_publication_evidence_identity> evidence);

  state_publication_outcome outcome_;
  state_storage_atomicity_boundary atomicity_;
  bool resulting_snapshot_established_;
  std::vector<state_publication_evidence_identity> evidence_;
};

/*!
 * \brief One exclusive lock-scoped canonical publication transaction.
 *
 * Construction must acquire the backend publication lock and reread complete
 * durable state under that lock.  Destruction releases the lock.  publish()
 * receives the only snapshot permitted by the validated request.
 */
class canonical_publication_transaction {
public:
  /*! \brief Release the publication lock. */
  virtual ~canonical_publication_transaction() = default;

  /*! \brief Return actual durable state observed under the lock. */
  [[nodiscard]] virtual const snapshot& current() const noexcept = 0;

  /*! \brief Return the line-safe canonical storage-format identifier. */
  [[nodiscard]] virtual const std::string& storage_format() const noexcept = 0;

  /*!
   * \brief Attempt publication of the one validated resulting snapshot.
   *
   * The method must not reinterpret or replace \p resulting_snapshot.  Every
   * ordinary attempt outcome after the prior snapshot was observed is returned
   * as state_publication_backend_result.
   */
  [[nodiscard]] virtual state_publication_backend_result
  publish(const snapshot& resulting_snapshot) = 0;
};

/*!
 * \brief Backend-neutral complete installed-state store.
 *
 * compare_and_publish() is non-virtual.  It acquires one lock-scoped backend
 * transaction, compares the actual snapshot identity, refuses stale requests
 * without mutation, derives the only permitted result, invokes publication,
 * and constructs the typed receipt.  Backends cannot override that sequence.
 */
class canonical_store {
public:
  /*! \brief Destroy the canonical store backend. */
  virtual ~canonical_store() = default;

  /*! \brief Read and validate one complete immutable installed snapshot. */
  [[nodiscard]] virtual snapshot read() const = 0;

  /*! \brief Compare under lock, publish when current, and return a receipt. */
  [[nodiscard]] state_publication_receipt
  compare_and_publish(const state_publication_request& request) const;

  /*! \brief Compare an empty destination and import one legacy observation. */
  [[nodiscard]] legacy_import_receipt
  import_legacy(const legacy_import_request& request) const;

protected:
  /*!
   * \brief Acquire the exclusive publication lock and reread durable state.
   * \throws store_error when locking or authoritative reread fails.
   */
  [[nodiscard]] virtual std::unique_ptr<canonical_publication_transaction>
  begin_publication() const = 0;
};

} // namespace pkgstate
