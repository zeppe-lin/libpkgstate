// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/digest.h>
#include <libpkgstate/legacy_completeness.h>
#include <libpkgstate/legacy_installed_package.h>
#include <libpkgstate/legacy_snapshot.h>

#include <type_traits>
#include <utility>
#include <vector>

#include "test.h"

namespace {

pkgstate::package_path
path(const char* value)
{
  return pkgstate::package_path::parse(value);
}

pkgstate::owned_entry
file(const char* value)
{
  return {path(value), pkgstate::owned_entry_type::non_directory};
}

pkgstate::owned_entry
directory(const char* value)
{
  return {path(value), pkgstate::owned_entry_type::directory};
}

pkgstate::legacy_installed_package
package(const char* name,
        const char* version,
        std::vector<pkgstate::owned_entry> manifest)
{
  return pkgstate::legacy_installed_package(
      pkgstate::package_identity::make(name, version),
      std::move(manifest));
}

} // namespace

int
main()
{
  static_assert(!std::is_same_v<
                pkgstate::legacy_package_observation_identity,
                pkgstate::installed_package_identity>);
  static_assert(!std::is_same_v<
                pkgstate::legacy_snapshot_observation_identity,
                pkgstate::installed_state_snapshot_identity>);

  constexpr pkgstate::legacy_package_completeness package_facts;
  CHECK(package_facts.package_name() ==
        pkgstate::legacy_fact_availability::retained_by_compatibility_format);
  CHECK(package_facts.opaque_version_line() ==
        pkgstate::legacy_fact_availability::retained_by_compatibility_format);
  CHECK(package_facts.ownership_manifest() ==
        pkgstate::legacy_fact_availability::retained_by_compatibility_format);
  CHECK(package_facts.ownership_object_class() ==
        pkgstate::legacy_fact_availability::retained_by_compatibility_format);
  CHECK(package_facts.package_observation_identity() ==
        pkgstate::legacy_fact_availability::derived_from_retained_facts);
  CHECK(package_facts.package_release() ==
        pkgstate::legacy_fact_availability::historically_unavailable);
  CHECK(package_facts.installed_control() ==
        pkgstate::legacy_fact_availability::historically_unavailable);
  CHECK(package_facts.state_target_binding() ==
        pkgstate::legacy_fact_availability::historically_unavailable);
  CHECK(package_facts.installed_package_identity() ==
        pkgstate::legacy_fact_availability::historically_unavailable);
  CHECK(package_facts.application_evidence() ==
        pkgstate::legacy_fact_availability::historically_unavailable);
  CHECK(package_facts.transaction_evidence() ==
        pkgstate::legacy_fact_availability::historically_unavailable);

  constexpr pkgstate::legacy_snapshot_completeness snapshot_facts;
  CHECK(snapshot_facts.package_records() ==
        pkgstate::legacy_fact_availability::retained_by_compatibility_format);
  CHECK(snapshot_facts.ownership_relation() ==
        pkgstate::legacy_fact_availability::derived_from_retained_facts);
  CHECK(snapshot_facts.package_observation_identities() ==
        pkgstate::legacy_fact_availability::derived_from_retained_facts);
  CHECK(snapshot_facts.snapshot_observation_identity() ==
        pkgstate::legacy_fact_availability::derived_from_retained_facts);
  CHECK(snapshot_facts.state_target_binding() ==
        pkgstate::legacy_fact_availability::historically_unavailable);
  CHECK(snapshot_facts.ownership_inventory_identity() ==
        pkgstate::legacy_fact_availability::historically_unavailable);
  CHECK(snapshot_facts.installed_state_snapshot_identity() ==
        pkgstate::legacy_fact_availability::historically_unavailable);
  CHECK(snapshot_facts.publication_history() ==
        pkgstate::legacy_fact_availability::historically_unavailable);

  const auto base = package(
      "base", "1.0-release-with-hyphens",
      {file("usr/bin/base"), directory("etc/base")});
  const auto reordered_base = package(
      "base", "1.0-release-with-hyphens",
      {directory("etc/base"), file("usr/bin/base")});
  const auto directory_base = package(
      "base", "1.0-release-with-hyphens",
      {directory("usr/bin/base"), directory("etc/base")});

  CHECK(base.completeness().package_release() ==
        pkgstate::legacy_fact_availability::historically_unavailable);
  CHECK(base.observation_identity() == reordered_base.observation_identity());
  CHECK(base.observation_identity() != directory_base.observation_identity());
  CHECK(base.observation_identity().string() ==
        "v1:sha256:79ddd2e709449ae3a438d3e7910000ae"
        "ac541cf5e0fa645f629cd852f40ce1c7");

  const pkgstate::legacy_snapshot empty;
  CHECK(empty.completeness().installed_state_snapshot_identity() ==
        pkgstate::legacy_fact_availability::historically_unavailable);
  CHECK(empty.observation_identity().string() ==
        "v1:sha256:8b5e8074f5813bc13e5d75fcbc6d1842"
        "6c8efc95c3053c51b48f3fe5e587f1f7");

  const pkgstate::legacy_snapshot state({
      package("zlib", "1.3-1", {file("usr/lib/libz.so")}),
      base,
      package("other", "2-1", {directory("etc/base")}),
  });
  const pkgstate::legacy_snapshot reordered_state({
      package("other", "2-1", {directory("etc/base")}),
      package("base", "1.0-release-with-hyphens",
              {directory("etc/base"), file("usr/bin/base")}),
      package("zlib", "1.3-1", {file("usr/lib/libz.so")}),
  });
  const pkgstate::legacy_snapshot changed_state({
      package("other", "2-1", {directory("etc/base")}),
      package("base", "1.0-release-with-hyphens",
              {directory("etc/base"), file("usr/bin/base")}),
      package("zlib", "1.3-2", {file("usr/lib/libz.so")}),
  });

  CHECK(state.observation_identity() == reordered_state.observation_identity());
  CHECK(state.observation_identity() != changed_state.observation_identity());
  CHECK(state.observation_identity().string() ==
        "v1:sha256:a4c6c480ee1b9ac1fb8474848cc59f844"
        "463a9ec723b4b6887997aee22c92421");

  const auto known_empty = package("empty", "1-1", {});
  CHECK(known_empty.manifest().empty());
  CHECK(known_empty.completeness().ownership_manifest() ==
        pkgstate::legacy_fact_availability::retained_by_compatibility_format);

  return 0;
}
