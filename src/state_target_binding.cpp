// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/state_target_binding.h>

#include "canonical_record.h"

#include <tuple>
#include <utility>

namespace pkgstate {
namespace {

state_target_binding_identity
identify_binding(const managed_target_identity& managed_target,
                 const state_store_identity& state_store,
                 const root_view_identity& root_view,
                 const state_backend_identity& state_backend,
                 const publication_domain_identity& publication_domain)
{
  detail::canonical_record record(
      state_target_binding_identity::canonical_domain());
  record.append_digest(managed_target);
  record.append_digest(state_store);
  record.append_digest(root_view);
  record.append_digest(state_backend);
  record.append_digest(publication_domain);
  return state_target_binding_identity::from_sha256(record.sha256());
}

} // namespace

state_target_binding
state_target_binding::make(managed_target_identity managed_target,
                           state_store_identity state_store,
                           root_view_identity root_view,
                           state_backend_identity state_backend,
                           publication_domain_identity publication_domain)
{
  const state_target_binding_identity identity = identify_binding(
      managed_target,
      state_store,
      root_view,
      state_backend,
      publication_domain);

  return state_target_binding(std::move(identity),
                              std::move(managed_target),
                              std::move(state_store),
                              std::move(root_view),
                              std::move(state_backend),
                              std::move(publication_domain));
}

state_target_binding::state_target_binding(
    state_target_binding_identity identity,
    managed_target_identity managed_target,
    state_store_identity state_store,
    root_view_identity root_view,
    state_backend_identity state_backend,
    publication_domain_identity publication_domain)
    : identity_(std::move(identity)),
      managed_target_(std::move(managed_target)),
      state_store_(std::move(state_store)),
      root_view_(std::move(root_view)),
      state_backend_(std::move(state_backend)),
      publication_domain_(std::move(publication_domain))
{
}

const state_target_binding_identity&
state_target_binding::identity() const noexcept
{
  return identity_;
}

const managed_target_identity&
state_target_binding::managed_target() const noexcept
{
  return managed_target_;
}

const state_store_identity&
state_target_binding::state_store() const noexcept
{
  return state_store_;
}

const root_view_identity&
state_target_binding::root_view() const noexcept
{
  return root_view_;
}

const state_backend_identity&
state_target_binding::state_backend() const noexcept
{
  return state_backend_;
}

const publication_domain_identity&
state_target_binding::publication_domain() const noexcept
{
  return publication_domain_;
}

bool
operator==(const state_target_binding& lhs,
           const state_target_binding& rhs) noexcept
{
  return lhs.identity_ == rhs.identity_ &&
         lhs.managed_target_ == rhs.managed_target_ &&
         lhs.state_store_ == rhs.state_store_ &&
         lhs.root_view_ == rhs.root_view_ &&
         lhs.state_backend_ == rhs.state_backend_ &&
         lhs.publication_domain_ == rhs.publication_domain_;
}

bool
operator!=(const state_target_binding& lhs,
           const state_target_binding& rhs) noexcept
{
  return !(lhs == rhs);
}

bool
operator<(const state_target_binding& lhs,
          const state_target_binding& rhs) noexcept
{
  return std::tie(lhs.managed_target_,
                  lhs.state_store_,
                  lhs.root_view_,
                  lhs.state_backend_,
                  lhs.publication_domain_,
                  lhs.identity_) <
         std::tie(rhs.managed_target_,
                  rhs.state_store_,
                  rhs.root_view_,
                  rhs.state_backend_,
                  rhs.publication_domain_,
                  rhs.identity_);
}

} // namespace pkgstate
