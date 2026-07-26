// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/installed_package.h>

#include "canonical_record.h"

#include <algorithm>
#include <utility>

namespace pkgstate {

installed_package installed_package::make(installation_receipt receipt)
{
  detail::canonical_record record(installed_package_identity::canonical_domain());
  record.append_digest(receipt.identity());
  return installed_package(
      installed_package_identity::from_sha256(record.sha256()),
      std::move(receipt));
}
installed_package::installed_package(installed_package_identity identity,
                                     installation_receipt receipt)
    : identity_(std::move(identity)), receipt_(std::move(receipt))
{
}
const installed_package_identity& installed_package::identity() const noexcept { return identity_; }
const installation_receipt& installed_package::receipt() const noexcept { return receipt_; }
const package_release& installed_package::release() const noexcept { return receipt_.release(); }
const installed_control& installed_package::control() const noexcept { return receipt_.control(); }
const state_target_binding& installed_package::target_binding() const noexcept { return receipt_.target_binding(); }
const std::vector<owned_entry>& installed_package::manifest() const noexcept { return receipt_.manifest(); }
std::size_t installed_package::size() const noexcept { return receipt_.manifest().size(); }
const owned_entry* installed_package::find(const package_path& path) const noexcept
{
  const auto found = std::lower_bound(
      manifest().begin(), manifest().end(), path,
      [](const owned_entry& entry, const package_path& wanted) {
        return entry.path() < wanted;
      });
  return found != manifest().end() && found->path() == path ? &*found : nullptr;
}
bool installed_package::owns(const package_path& path) const noexcept { return find(path) != nullptr; }
bool operator==(const installed_package& lhs, const installed_package& rhs) noexcept { return lhs.identity_ == rhs.identity_ && lhs.receipt_ == rhs.receipt_; }
bool operator!=(const installed_package& lhs, const installed_package& rhs) noexcept { return !(lhs == rhs); }
bool operator<(const installed_package& lhs, const installed_package& rhs) noexcept { return lhs.release().package() < rhs.release().package(); }

} // namespace pkgstate
