// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/libpkgstate.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>

namespace {

template<typename Identity>
Identity
identity(char digit)
{
  return Identity::parse("v1:sha256:" + std::string(64, digit));
}

pkgstate::state_target_binding
binding()
{
  return pkgstate::state_target_binding::make(
      identity<pkgstate::managed_target_identity>('1'),
      identity<pkgstate::state_store_identity>('2'),
      identity<pkgstate::root_view_identity>('3'),
      identity<pkgstate::state_backend_identity>('4'),
      identity<pkgstate::publication_domain_identity>('5'));
}

pkgstate::installed_package
installed(const pkgstate::state_target_binding& target)
{
  const pkgstate::package_release release =
      pkgstate::package_release::make("base", "1.0", "1");

  pkgstate::installed_control_completeness completeness;
  completeness.runtime_dependencies =
      pkgstate::installed_control_fact_state::recorded_at_installation;
  completeness.removal_lifecycle =
      pkgstate::installed_control_fact_state::
          recorded_in_compatibility_storage;
  completeness.target_profile =
      pkgstate::installed_control_fact_state::historically_unavailable;
  completeness.provenance =
      pkgstate::installed_control_fact_state::supplied_by_migration;

  pkgstate::installed_control control = pkgstate::installed_control::make(
      release,
      completeness,
      {pkgstate::runtime_dependency_declaration::make("libc")},
      {pkgstate::removal_lifecycle_declaration::make(
          pkgstate::removal_lifecycle_phase::pre_remove,
          "application/x-posix-shell",
          "exit 0\n")},
      {},
      {pkgstate::control_provenance::make(
          pkgstate::control_provenance_kind::application_evidence,
          identity<pkgstate::application_evidence_identity>('a').string())});

  return pkgstate::installed_package::make(
      release,
      std::move(control),
      target,
      {
          {pkgstate::package_path::parse("usr"),
           pkgstate::owned_entry_type::directory},
          {pkgstate::package_path::parse("usr/bin/base"),
           pkgstate::owned_entry_type::non_directory},
      });
}

} // namespace

int
main(int argc, char** argv)
{
  if (argc != 2)
  {
    std::cerr << "usage: pkgstate-check-fixture canonical-store\n";
    return EXIT_FAILURE;
  }

  try
  {
    const pkgstate::state_target_binding target = binding();
    pkgstate::canonical_generation_store store(
        std::filesystem::path(argv[1]), target);
    const pkgstate::installed_package package = installed(target);
    const pkgstate::state_publication_request request =
        pkgstate::state_publication_request::make(
            store.read(),
            {pkgstate::package_state_delta::install(
                package,
                identity<pkgstate::operation_plan_identity>('b'),
                identity<pkgstate::application_evidence_identity>('a'))});
    const pkgstate::state_publication_receipt receipt =
        store.compare_and_publish(request);
    if (receipt.outcome() !=
        pkgstate::state_publication_outcome::published)
    {
      std::cerr << "fixture publication did not complete\n";
      return EXIT_FAILURE;
    }
  }
  catch (const std::exception& failure)
  {
    std::cerr << "pkgstate-check-fixture: " << failure.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
