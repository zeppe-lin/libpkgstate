// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "publication_projection.h"

#include <algorithm>
#include <string_view>
#include <utility>
#include <vector>

#include <libpkgstate/error.h>
#include <libpkgstate/installed_package.h>

namespace pkgstate::detail {

snapshot
project_publication_request(const state_publication_request& request,
                            const snapshot& actual_prior)
{
  if (actual_prior.target_binding() != request.target_binding())
    throw state_error("publication request belongs to another target");

  if (actual_prior.identity() != request.expected_snapshot())
    throw state_error("publication request does not match actual prior state");

  std::vector<installed_package> packages = actual_prior.packages();

  for (const package_state_delta& delta : request.deltas())
  {
    const auto position = std::lower_bound(
        packages.begin(), packages.end(), delta.package_name(),
        [](const installed_package& package, std::string_view name) {
          return package.release().name() < name;
        });

    const bool present =
        position != packages.end() &&
        position->release().name() == delta.package_name();

    switch (delta.kind())
    {
      case package_state_delta_kind::install:
        if (present)
          throw state_error("publication install package is already present");
        packages.insert(position, *delta.proposed_package());
        break;

      case package_state_delta_kind::replace:
        if (!present || position->identity() != *delta.expected_package())
        {
          throw state_error(
              "publication replacement prior package does not match");
        }
        *position = *delta.proposed_package();
        break;

      case package_state_delta_kind::remove:
        if (!present || position->identity() != *delta.expected_package())
        {
          throw state_error("publication removal prior package does not match");
        }
        packages.erase(position);
        break;
    }
  }

  return snapshot::make(request.target_binding(), std::move(packages));
}

} // namespace pkgstate::detail
