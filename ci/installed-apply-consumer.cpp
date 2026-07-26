// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate-apply/adapter.h>

#include <string>

int
main()
{
  const pkgstate::apply_adapter::projection_error error(
      pkgstate::apply_adapter::projection_error_code::request_binding_mismatch,
      "consumer probe");
  return error.code() ==
                 pkgstate::apply_adapter::projection_error_code::
                     request_binding_mismatch &&
             std::string(error.what()) == "consumer probe"
         ? 0
         : 1;
}
