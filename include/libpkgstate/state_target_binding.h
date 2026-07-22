// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file state_target_binding.h
 * \brief Durable binding between installed state and its target domain.
 */

#pragma once

#include <libpkgstate/digest.h>

namespace pkgstate {

/*!
 * \brief Durable state projection of one managed package target.
 *
 * Component identities are supplied by their owning authorities. libpkgstate
 * computes the binding identity from those exact typed identities. The value
 * deliberately contains neither a root pathname nor an installed snapshot
 * identity.
 */
class state_target_binding final {
public:
  /*!
   * \brief Construct and identify one durable target-state binding.
   * \param managed_target Managed package-target identity.
   * \param state_store Durable installed-state store identity.
   * \param root_view Logical target root-view identity.
   * \param state_backend Installed-state backend identity.
   * \param publication_domain Publication and locking-domain identity.
   * \return Canonical binding of the five supplied identities.
   * \throws state_error when canonical identity computation fails.
   */
  [[nodiscard]] static state_target_binding
  make(managed_target_identity managed_target,
       state_store_identity state_store,
       root_view_identity root_view,
       state_backend_identity state_backend,
       publication_domain_identity publication_domain);

  /*! \brief Return the computed target-binding identity. */
  [[nodiscard]] const state_target_binding_identity& identity() const noexcept;

  /*! \brief Return the managed package-target identity. */
  [[nodiscard]] const managed_target_identity& managed_target() const noexcept;

  /*! \brief Return the durable installed-state store identity. */
  [[nodiscard]] const state_store_identity& state_store() const noexcept;

  /*! \brief Return the logical target root-view identity. */
  [[nodiscard]] const root_view_identity& root_view() const noexcept;

  /*! \brief Return the installed-state backend identity. */
  [[nodiscard]] const state_backend_identity& state_backend() const noexcept;

  /*! \brief Return the publication and locking-domain identity. */
  [[nodiscard]] const publication_domain_identity&
  publication_domain() const noexcept;

  friend bool operator==(const state_target_binding& lhs,
                         const state_target_binding& rhs) noexcept;
  friend bool operator!=(const state_target_binding& lhs,
                         const state_target_binding& rhs) noexcept;
  friend bool operator<(const state_target_binding& lhs,
                        const state_target_binding& rhs) noexcept;

private:
  state_target_binding(state_target_binding_identity identity,
                       managed_target_identity managed_target,
                       state_store_identity state_store,
                       root_view_identity root_view,
                       state_backend_identity state_backend,
                       publication_domain_identity publication_domain);

  state_target_binding_identity identity_;
  managed_target_identity managed_target_;
  state_store_identity state_store_;
  root_view_identity root_view_;
  state_backend_identity state_backend_;
  publication_domain_identity publication_domain_;
};

} // namespace pkgstate
