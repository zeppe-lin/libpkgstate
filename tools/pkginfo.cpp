// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file pkginfo.cpp
 * \brief Reference command-line inspector for libpkgstate.
 */

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <getopt.h>

#include <libpkgimage/error.h>
#include <libpkgimage/package_path.h>
#include <libpkgstate/error.h>
#include <libpkgstate/legacy_text_store.h>

#ifndef PKGSTATE_VERSION
#define PKGSTATE_VERSION "unknown"
#endif

namespace {

constexpr int usage_status = 2;

enum class action {
  none,
  installed,
  list,
  owner,
};

struct options final {
  action selected = action::none;
  std::filesystem::path root = "/";
  std::string argument;
};

/*!
 * \brief Print command usage.
 * \param out Destination stream.
 */
void
print_help(std::ostream& out)
{
  out << R"(Usage: pkginfo [-Vh] [-r root-dir] {-i | -l package | -o path}
Inspect durable installed package state.

Exactly one query mode is required:
  -i, --installed           List installed packages and versions
  -l, --list=package        List paths owned by an installed package
  -o, --owner=path          List packages that own an exact path

Other options:
  -r, --root=root-dir       Read root-dir/var/lib/pkg/db
  -V, --version             Print version and exit
  -h, --help                Print this help and exit

Paths printed by --list are canonical and root-relative.  Directory ownership
is shown with a trailing slash.  --owner accepts either root-relative spelling
or one or more leading slashes.

This is a libpkgstate reference tool.  It is not the package-management
interface provided by pkgman.
)";
}

/*!
 * \brief Print reference-tool version information.
 */
void
print_version()
{
  std::cout << "pkginfo (libpkgstate) " << PKGSTATE_VERSION << '\n';
}

/*!
 * \brief Select one query action.
 * \throws std::invalid_argument if more than one action is requested.
 */
void
select_action(options& parsed, action selected, std::string argument = {})
{
  if (parsed.selected != action::none)
    throw std::invalid_argument("exactly one query mode is required");

  parsed.selected = selected;
  parsed.argument = std::move(argument);
}

/*!
 * \brief Parse command-line options.
 * \return Parsed query options.  Help and version requests exit directly.
 */
options
parse_options(int argc, char** argv)
{
  options parsed;

  static const option long_options[] = {
    {"installed", no_argument, nullptr, 'i'},
    {"list", required_argument, nullptr, 'l'},
    {"owner", required_argument, nullptr, 'o'},
    {"root", required_argument, nullptr, 'r'},
    {"version", no_argument, nullptr, 'V'},
    {"help", no_argument, nullptr, 'h'},
    {nullptr, 0, nullptr, 0},
  };

  opterr = 0;
  for (;;)
  {
    const int option_value =
        getopt_long(argc, argv, "il:o:r:Vh", long_options, nullptr);
    if (option_value == -1)
      break;

    switch (option_value)
    {
      case 'i':
        select_action(parsed, action::installed);
        break;
      case 'l':
        select_action(parsed, action::list, optarg);
        break;
      case 'o':
        select_action(parsed, action::owner, optarg);
        break;
      case 'r':
        parsed.root = optarg;
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
    throw std::invalid_argument("unexpected positional argument: " +
                                std::string(argv[optind]));

  if (parsed.selected == action::none)
    throw std::invalid_argument("query mode is required");

  if (parsed.root.empty())
    throw std::invalid_argument("root directory must not be empty");

  return parsed;
}

/*!
 * \brief Resolve the historical database location beneath an alternate root.
 */
std::filesystem::path
database_path(const std::filesystem::path& root)
{
  return root / "var/lib/pkg/db";
}

/*!
 * \brief Parse a user-facing owned path.
 *
 * The state model uses root-relative package paths.  The tool also accepts
 * conventional absolute spelling and removes only leading separators before
 * applying package-path validation.
 */
pkgimage::package_path
parse_owned_path(std::string_view input)
{
  while (!input.empty() && input.front() == '/')
    input.remove_prefix(1);

  return pkgimage::package_path::parse(input);
}

/*!
 * \brief Print every installed package and version.
 */
void
print_installed(const pkgstate::snapshot& state)
{
  for (const pkgstate::installed_package& package : state.packages())
  {
    std::cout << package.identity().name() << ' '
              << package.identity().version() << '\n';
  }
}

/*!
 * \brief Print the canonical ownership manifest of one package.
 * \throws std::runtime_error when the package is not installed.
 */
void
print_manifest(const pkgstate::snapshot& state, std::string_view name)
{
  const pkgstate::installed_package* package = state.find_package(name);
  if (package == nullptr)
    throw std::runtime_error("package is not installed: " + std::string(name));

  for (const pkgstate::owned_entry& entry : package->manifest())
  {
    std::cout << entry.path.string();
    if (entry.type == pkgstate::owned_entry_type::directory)
      std::cout << '/';
    std::cout << '\n';
  }
}

/*!
 * \brief Print every package that owns an exact path.
 * \return True when at least one owner was found.
 */
bool
print_owners(const pkgstate::snapshot& state, std::string_view input)
{
  const pkgimage::package_path path = parse_owned_path(input);
  const auto owners = state.owners(path);

  for (const pkgstate::installed_package* package : owners)
  {
    std::cout << package->identity().name() << ' '
              << package->identity().version() << '\n';
  }

  return !owners.empty();
}

} // namespace

/*!
 * \brief Run the libpkgstate reference inspector.
 */
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
    std::cerr << "pkginfo: " << failure.what() << '\n';
    print_help(std::cerr);
    return usage_status;
  }

  try
  {
    const pkgstate::legacy_text_store store(database_path(parsed.root));
    const pkgstate::snapshot state = store.read();

    switch (parsed.selected)
    {
      case action::installed:
        print_installed(state);
        return EXIT_SUCCESS;
      case action::list:
        print_manifest(state, parsed.argument);
        return EXIT_SUCCESS;
      case action::owner:
        if (print_owners(state, parsed.argument))
          return EXIT_SUCCESS;
        std::cerr << "pkginfo: no package owns: " << parsed.argument << '\n';
        return EXIT_FAILURE;
      case action::none:
        break;
    }
  }
  catch (const pkgstate::error& failure)
  {
    std::cerr << "pkginfo: " << failure.what() << '\n';
    return EXIT_FAILURE;
  }
  catch (const pkgimage::error& failure)
  {
    std::cerr << "pkginfo: " << failure.what() << '\n';
    return EXIT_FAILURE;
  }
  catch (const std::exception& failure)
  {
    std::cerr << "pkginfo: " << failure.what() << '\n';
    return EXIT_FAILURE;
  }

  std::cerr << "pkginfo: internal action dispatch failure\n";
  return EXIT_FAILURE;
}
