// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later
#include <libpkgstate-source/adapter.h>
#include <string>
int main()
{
  const pkgstate::source_adapter::projection_error error(
      pkgstate::source_adapter::projection_error_code::record_construction,
      "consumer probe");
  return error.code() ==
                 pkgstate::source_adapter::projection_error_code::
                     record_construction &&
             std::string(error.what()) == "consumer probe"
         ? 0
         : 1;
}
