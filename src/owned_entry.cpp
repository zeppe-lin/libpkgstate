// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/owned_entry.h>

namespace pkgstate {

bool
operator==(const owned_entry& lhs, const owned_entry& rhs) noexcept
{
  return lhs.path == rhs.path && lhs.type == rhs.type;
}

bool
operator!=(const owned_entry& lhs, const owned_entry& rhs) noexcept
{
  return !(lhs == rhs);
}

} // namespace pkgstate
