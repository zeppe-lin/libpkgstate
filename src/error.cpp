// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/error.h>

#include <utility>

namespace pkgstate {

error::error(std::string message)
    : std::runtime_error(std::move(message))
{
}

error::~error() = default;
identity_error::~identity_error() = default;
path_error::~path_error() = default;
state_error::~state_error() = default;
store_error::~store_error() = default;

} // namespace pkgstate
