// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file pkgstate_check.cpp
 * \brief Read-only diagnostics for canonical and compatibility state.
 */

#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <getopt.h>

#include <libpkgstate/canonical_generation_store.h>
#include <libpkgstate/error.h>
#include <libpkgstate/installed_control.h>
#include <libpkgstate/legacy_completeness.h>
#include <libpkgstate/legacy_text_store.h>

#ifndef PKGSTATE_VERSION
#define PKGSTATE_VERSION "unknown"
#endif

namespace {

constexpr int usage_status = 2;

enum class diagnostic_mode {
  none,
  canonical,
  legacy,
};

enum long_option_value {
  managed_target_option = 1000,
  state_store_option,
  root_view_option,
  state_backend_option,
  publication_domain_option,
};

struct options final {
  diagnostic_mode mode = diagnostic_mode::none;
  std::filesystem::path path;
  std::optional<std::string> managed_target;
  std::optional<std::string> state_store;
  std::optional<std::string> root_view;
  std::optional<std::string> state_backend;
  std::optional<std::string> publication_domain;
};

struct ownership_summary final {
  std::size_t claims = 0;
  std::size_t paths = 0;
  std::size_t shared_paths = 0;
};

struct control_summary final {
  std::size_t recorded_at_installation = 0;
  std::size_t recorded_in_compatibility_storage = 0;
  std::size_t supplied_by_migration = 0;
  std::size_t historically_unavailable = 0;
};

void
print_help(std::ostream& out)
{
  out << R"(Usage:
  pkgstate-check --canonical-store path \
    --managed-target identity --state-store identity \
    --root-view identity --state-backend identity \
    --publication-domain identity
  pkgstate-check --legacy-database path
  pkgstate-check {-V | -h}

Validate and summarize durable package state without modifying it.

Modes:
  -c, --canonical-store=path  Inspect an existing canonical generation store
  -l, --legacy-database=path  Inspect a historical line-oriented database

Canonical target binding:
      --managed-target=id      Managed package-target identity
      --state-store=id         Durable installed-state store identity
      --root-view=id           Logical target root-view identity
      --state-backend=id       Installed-state backend identity
      --publication-domain=id  Publication and locking-domain identity

Other options:
  -V, --version                Print version and exit
  -h, --help                   Print this help and exit

The command is read-only.  It never initializes a canonical store, completes
recovery, publishes a generation, imports compatibility state, repairs data, or
opens a compatibility write transaction.
)";
}

void
print_version()
{
  std::cout << "pkgstate-check (libpkgstate) " << PKGSTATE_VERSION << '\n';
}

void
select_mode(options& parsed,
            diagnostic_mode selected,
            std::filesystem::path path)
{
  if (parsed.mode != diagnostic_mode::none)
    throw std::invalid_argument("exactly one diagnostic mode is required");
  if (path.empty())
    throw std::invalid_argument("state pathname must not be empty");

  parsed.mode = selected;
  parsed.path = std::move(path);
}

options
parse_options(int argc, char** argv)
{
  options parsed;
  static const option long_options[] = {
      {"canonical-store", required_argument, nullptr, 'c'},
      {"legacy-database", required_argument, nullptr, 'l'},
      {"managed-target", required_argument, nullptr, managed_target_option},
      {"state-store", required_argument, nullptr, state_store_option},
      {"root-view", required_argument, nullptr, root_view_option},
      {"state-backend", required_argument, nullptr, state_backend_option},
      {"publication-domain", required_argument, nullptr,
       publication_domain_option},
      {"version", no_argument, nullptr, 'V'},
      {"help", no_argument, nullptr, 'h'},
      {nullptr, 0, nullptr, 0},
  };

  opterr = 0;
  for (;;)
  {
    const int option_value =
        getopt_long(argc, argv, "c:l:Vh", long_options, nullptr);
    if (option_value == -1)
      break;

    switch (option_value)
    {
      case 'c':
        select_mode(parsed, diagnostic_mode::canonical, optarg);
        break;
      case 'l':
        select_mode(parsed, diagnostic_mode::legacy, optarg);
        break;
      case managed_target_option:
        parsed.managed_target = optarg;
        break;
      case state_store_option:
        parsed.state_store = optarg;
        break;
      case root_view_option:
        parsed.root_view = optarg;
        break;
      case state_backend_option:
        parsed.state_backend = optarg;
        break;
      case publication_domain_option:
        parsed.publication_domain = optarg;
        break;
      case 'V':
        print_version();
        std::exit(EXIT_SUCCESS);
      case 'h':
        print_help(std::cout);
        std::exit(EXIT_SUCCESS);
      case '?':
      default:
        throw std::invalid_argument("invalid command-line option");
    }
  }

  if (optind != argc)
  {
    throw std::invalid_argument("unexpected positional argument: " +
                                std::string(argv[optind]));
  }
  if (parsed.mode == diagnostic_mode::none)
    throw std::invalid_argument("diagnostic mode is required");

  const bool has_binding = parsed.managed_target.has_value() ||
                           parsed.state_store.has_value() ||
                           parsed.root_view.has_value() ||
                           parsed.state_backend.has_value() ||
                           parsed.publication_domain.has_value();
  if (parsed.mode == diagnostic_mode::legacy && has_binding)
  {
    throw std::invalid_argument(
        "target-binding options require --canonical-store");
  }
  if (parsed.mode == diagnostic_mode::canonical &&
      (!parsed.managed_target || !parsed.state_store || !parsed.root_view ||
       !parsed.state_backend || !parsed.publication_domain))
  {
    throw std::invalid_argument(
        "canonical diagnostics require all target-binding identities");
  }

  return parsed;
}

template<typename Package>
ownership_summary
summarize_ownership(const std::vector<Package>& packages)
{
  ownership_summary result;
  std::map<std::string, std::size_t> owners_by_path;

  for (const Package& package : packages)
  {
    result.claims += package.manifest().size();
    for (const pkgstate::owned_entry& entry : package.manifest())
      ++owners_by_path[entry.path.string()];
  }

  result.paths = owners_by_path.size();
  for (const auto& path_owners : owners_by_path)
  {
    if (path_owners.second > 1)
      ++result.shared_paths;
  }
  return result;
}

void
count_control_state(control_summary& result,
                    pkgstate::installed_control_fact_state state)
{
  switch (state)
  {
    case pkgstate::installed_control_fact_state::recorded_at_installation:
      ++result.recorded_at_installation;
      return;
    case pkgstate::installed_control_fact_state::
        recorded_in_compatibility_storage:
      ++result.recorded_in_compatibility_storage;
      return;
    case pkgstate::installed_control_fact_state::supplied_by_migration:
      ++result.supplied_by_migration;
      return;
    case pkgstate::installed_control_fact_state::historically_unavailable:
      ++result.historically_unavailable;
      return;
  }

  throw std::logic_error("unknown installed-control fact state");
}

control_summary
summarize_control(const pkgstate::snapshot& state)
{
  control_summary result;
  for (const pkgstate::installed_package& package : state.packages())
  {
    const pkgstate::installed_control_completeness& completeness =
        package.control().completeness();
    count_control_state(result, completeness.runtime_dependencies);
    count_control_state(result, completeness.removal_lifecycle);
    count_control_state(result, completeness.target_profile);
    count_control_state(result, completeness.provenance);
  }
  return result;
}

const char*
availability_name(pkgstate::legacy_fact_availability availability)
{
  switch (availability)
  {
    case pkgstate::legacy_fact_availability::
        retained_by_compatibility_format:
      return "retained-by-compatibility-format";
    case pkgstate::legacy_fact_availability::derived_from_retained_facts:
      return "derived-from-retained-facts";
    case pkgstate::legacy_fact_availability::historically_unavailable:
      return "historically-unavailable";
  }

  throw std::logic_error("unknown legacy fact availability");
}

void
print_path(std::string_view key, const std::filesystem::path& path)
{
  std::cout << key << '=' << std::quoted(path.string()) << '\n';
}

void
print_ownership(const ownership_summary& summary)
{
  std::cout << "ownership-claims=" << summary.claims << '\n'
            << "owned-paths=" << summary.paths << '\n'
            << "shared-paths=" << summary.shared_paths << '\n';
}

pkgstate::state_target_binding
parse_binding(const options& parsed)
{
  return pkgstate::state_target_binding::make(
      pkgstate::managed_target_identity::parse(*parsed.managed_target),
      pkgstate::state_store_identity::parse(*parsed.state_store),
      pkgstate::root_view_identity::parse(*parsed.root_view),
      pkgstate::state_backend_identity::parse(*parsed.state_backend),
      pkgstate::publication_domain_identity::parse(
          *parsed.publication_domain));
}

void
inspect_canonical(const options& parsed)
{
  const pkgstate::state_target_binding expected = parse_binding(parsed);
  pkgstate::canonical_generation_store store =
      pkgstate::canonical_generation_store::open_existing(
          parsed.path, expected);
  const pkgstate::snapshot state = store.read();
  const ownership_summary ownership = summarize_ownership(state.packages());
  const control_summary control = summarize_control(state);
  const pkgstate::state_target_binding& target = state.target_binding();

  std::cout << "mode=canonical\n"
            << "status=valid\n";
  print_path("store", parsed.path);
  std::cout << "storage-format="
            << pkgstate::canonical_generation_storage_format << '\n'
            << "target-binding=" << target.identity().string() << '\n'
            << "managed-target=" << target.managed_target().string() << '\n'
            << "state-store=" << target.state_store().string() << '\n'
            << "root-view=" << target.root_view().string() << '\n'
            << "state-backend=" << target.state_backend().string() << '\n'
            << "publication-domain="
            << target.publication_domain().string() << '\n'
            << "snapshot=" << state.identity().string() << '\n'
            << "ownership-inventory="
            << state.ownership_identity().string() << '\n'
            << "packages=" << state.size() << '\n';
  print_ownership(ownership);
  std::cout << "control.recorded-at-installation="
            << control.recorded_at_installation << '\n'
            << "control.recorded-in-compatibility-storage="
            << control.recorded_in_compatibility_storage << '\n'
            << "control.supplied-by-migration="
            << control.supplied_by_migration << '\n'
            << "control.historically-unavailable="
            << control.historically_unavailable << '\n';
}

void
print_legacy_completeness()
{
  const pkgstate::legacy_package_completeness package;
  const pkgstate::legacy_snapshot_completeness snapshot;

  std::cout
      << "package-fact.package-name="
      << availability_name(package.package_name()) << '\n'
      << "package-fact.opaque-version-line="
      << availability_name(package.opaque_version_line()) << '\n'
      << "package-fact.ownership-manifest="
      << availability_name(package.ownership_manifest()) << '\n'
      << "package-fact.ownership-object-class="
      << availability_name(package.ownership_object_class()) << '\n'
      << "package-fact.observation-identity="
      << availability_name(package.package_observation_identity()) << '\n'
      << "package-fact.package-release="
      << availability_name(package.package_release()) << '\n'
      << "package-fact.installed-control="
      << availability_name(package.installed_control()) << '\n'
      << "package-fact.target-binding="
      << availability_name(package.state_target_binding()) << '\n'
      << "package-fact.installed-package-identity="
      << availability_name(package.installed_package_identity()) << '\n'
      << "package-fact.application-evidence="
      << availability_name(package.application_evidence()) << '\n'
      << "package-fact.transaction-evidence="
      << availability_name(package.transaction_evidence()) << '\n'
      << "snapshot-fact.package-records="
      << availability_name(snapshot.package_records()) << '\n'
      << "snapshot-fact.ownership-relation="
      << availability_name(snapshot.ownership_relation()) << '\n'
      << "snapshot-fact.package-observation-identities="
      << availability_name(snapshot.package_observation_identities()) << '\n'
      << "snapshot-fact.observation-identity="
      << availability_name(snapshot.snapshot_observation_identity()) << '\n'
      << "snapshot-fact.target-binding="
      << availability_name(snapshot.state_target_binding()) << '\n'
      << "snapshot-fact.ownership-inventory-identity="
      << availability_name(snapshot.ownership_inventory_identity()) << '\n'
      << "snapshot-fact.installed-state-snapshot-identity="
      << availability_name(snapshot.installed_state_snapshot_identity()) << '\n'
      << "snapshot-fact.publication-history="
      << availability_name(snapshot.publication_history()) << '\n';
}

void
inspect_legacy(const options& parsed)
{
  const pkgstate::legacy_text_store store(parsed.path);
  const pkgstate::legacy_snapshot state = store.read();
  const ownership_summary ownership = summarize_ownership(state.packages());

  std::cout << "mode=legacy\n"
            << "status=valid\n";
  print_path("database", parsed.path);
  std::cout << "snapshot-observation="
            << state.observation_identity().string() << '\n'
            << "packages=" << state.size() << '\n';
  print_ownership(ownership);
  print_legacy_completeness();
}

} // namespace

int
main(int argc, char** argv)
{
  options parsed;
  try
  {
    parsed = parse_options(argc, argv);
  }
  catch (const std::invalid_argument& failure)
  {
    std::cerr << "pkgstate-check: " << failure.what() << '\n';
    print_help(std::cerr);
    return usage_status;
  }

  try
  {
    switch (parsed.mode)
    {
      case diagnostic_mode::canonical:
        inspect_canonical(parsed);
        return EXIT_SUCCESS;
      case diagnostic_mode::legacy:
        inspect_legacy(parsed);
        return EXIT_SUCCESS;
      case diagnostic_mode::none:
        break;
    }
  }
  catch (const pkgstate::error& failure)
  {
    std::cerr << "pkgstate-check: " << failure.what() << '\n';
    return EXIT_FAILURE;
  }
  catch (const std::exception& failure)
  {
    std::cerr << "pkgstate-check: " << failure.what() << '\n';
    return EXIT_FAILURE;
  }

  std::cerr << "pkgstate-check: internal diagnostic dispatch failure\n";
  return EXIT_FAILURE;
}
