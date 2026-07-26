// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate-build/adapter.h>

#include <string>

int main()
{
  const pkgstate::build_adapter::projection_error error(
      pkgstate::build_adapter::projection_error_code::source_binding,
      "consumer probe");
  return error.code() ==
                 pkgstate::build_adapter::projection_error_code::source_binding &&
             std::string(error.what()) == "consumer probe"
         ? 0
         : 1;
}
