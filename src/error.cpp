/*
 * Copyright (C) 2026 Alexandr Savca
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <libpkgstate/error.h>

namespace pkgstate {

error::error(const std::string& message)
    : std::runtime_error(message)
{
}

} // namespace pkgstate
