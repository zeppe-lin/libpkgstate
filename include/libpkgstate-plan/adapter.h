// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file adapter.h
 * \brief Explicit projection of canonical installed state into libpkgplan.
 */

#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include <libpkgplan/digest.h>
#include <libpkgplan/ownership.h>
#include <libpkgplan/package_fact.h>
#include <libpkgstate/snapshot.h>

namespace pkgstate::plan_adapter {

/*! \brief Structured reason that installed-state projection failed. */
enum class projection_error_code {
  target_binding_mismatch, //!< Caller target projection does not match state.
  identity_translation,    //!< Identity wire representation was incompatible.
  path_translation,        //!< Canonical path vocabularies were incompatible.
};

/*! \brief Failure to project canonical state into planner-owned fact types. */
class projection_error final : public std::invalid_argument {
public:
  /*! \brief Construct a typed projection failure. */
  projection_error(projection_error_code code, std::string message);

  /*! \brief Return the machine-readable failure class. */
  [[nodiscard]] projection_error_code code() const noexcept;

private:
  projection_error_code code_;
};

/*!
 * \brief Caller-authoritative planner target and its durable state projection.
 *
 * libpkgstate cannot derive a complete planner target-system context from an
 * installed snapshot.  The caller supplies that context identity together with
 * the exact durable state projection it claims belongs to the context.
 */
class planning_target_context final {
public:
  /*! \brief Bind a planner context to its durable state projection. */
  planning_target_context(
      pkgplan::target_system_context_identity identity,
      state_target_binding state_projection);

  /*! \brief Return the caller-authoritative planner context identity. */
  [[nodiscard]] const pkgplan::target_system_context_identity&
  identity() const noexcept;

  /*! \brief Return the claimed durable state projection. */
  [[nodiscard]] const state_target_binding&
  state_projection() const noexcept;

private:
  pkgplan::target_system_context_identity identity_;
  state_target_binding state_projection_;
};

/*!
 * \brief Planner-owned fact projection of one complete canonical snapshot.
 *
 * Package facts remain in snapshot package-name order.  Ownership claims are
 * normalized by libpkgplan.  The adapter supplies no filesystem metadata that
 * canonical installed state does not represent in planner vocabulary.
 */
class installed_state_projection final {
public:
  /*! \brief Return the caller-authoritative complete target context. */
  [[nodiscard]] const pkgplan::target_system_context_identity&
  target() const noexcept;

  /*! \brief Return installed package facts in package-name order. */
  [[nodiscard]] const std::vector<pkgplan::installed_package_fact>&
  packages() const noexcept;

  /*! \brief Return the complete installed path-to-owner relation. */
  [[nodiscard]] const pkgplan::installed_ownership_inventory&
  ownership() const noexcept;

private:
  friend installed_state_projection project_installed_state(
      const snapshot&, const planning_target_context&);

  installed_state_projection(
      pkgplan::target_system_context_identity target,
      std::vector<pkgplan::installed_package_fact> packages,
      pkgplan::installed_ownership_inventory ownership);

  pkgplan::target_system_context_identity target_;
  std::vector<pkgplan::installed_package_fact> packages_;
  pkgplan::installed_ownership_inventory ownership_;
};

/*!
 * \brief Project complete canonical installed truth into libpkgplan facts.
 *
 * The function accepts only pkgstate::snapshot, never legacy_snapshot.  It
 * verifies the caller's durable target projection before copying canonical
 * identity representations into the matching planner domains.
 *
 * \throws projection_error when the target or vocabularies disagree.
 */
[[nodiscard]] installed_state_projection
project_installed_state(const snapshot& state,
                        const planning_target_context& target);

} // namespace pkgstate::plan_adapter
