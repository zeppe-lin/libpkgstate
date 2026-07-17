libpkgstate
===========

`libpkgstate` is a C++17 library for representing and persisting durable
installed package state.

It provides:

* validated package identities;
* canonical sorted installed ownership manifests with directory identity;
* immutable indexed snapshots;
* package lookup and shared-ownership queries;
* backend-neutral state write transactions; and
* an original compatibility backend for the historical `/var/lib/pkg/db`
  text format.

An installed manifest describes what a package owns after installation policy
has been resolved.  It is not a copy of the package archive manifest.

`libpkgstate` records installed-state truth.  It does not inspect archives,
apply install policy, mutate the live filesystem, stage rejected files, run
system maintenance, or coordinate complete package transactions.

The implementation is original Zeppe-Lin code.  It is not derived from CRUX
`pkgutils` or from the former CRUX-derived implementation in `libpkgcore`.

Requirements
------------

Build-time requirements:

* a C++17 compiler;
* Meson 1.2.0 or later;
* Ninja;
* pkg-config; and
* `libpkgimage` 0.2.1 or later.

Doxygen is optional and is only needed to build the generated API reference.

Building
--------

Shared library:

```sh
meson setup build
meson compile -C build
meson test -C build
```

Static library with static dependencies:

```sh
meson setup build-static \
  -Ddefault_library=static \
  -Dlink_mode=static
meson compile -C build-static
meson test -C build-static
```

Installation:

```sh
meson install -C build
```

The project intentionally rejects `default_library=both`.  Shared and static
artifacts are separate builds, and each build links the corresponding form of
its dependencies.

Tests may be disabled with:

```sh
meson setup build -Dtests=disabled
```

Reference tools are built by default but are not installed.  They may be
disabled, or explicitly installed, with:

```sh
meson setup build -Dtools=disabled
meson setup build-tools -Dinstall_tools=true
```

State and storage contracts
---------------------------

The central distinction is:

```text
pkgimage::package_image       what an archive contains
pkgstate::installed_package   what durable state says is installed
```

Installed manifests are sorted ownership sets that retain the durable directory/non-directory distinction.  Shared ownership is retained
and queryable; it is not silently collapsed or rejected.

`write_transaction` isolates mutations to package-state storage.  It is not a
filesystem transaction and does not claim atomicity across installed files and
the package database.

The `legacy_text_store` backend implements the existing line-oriented database
format.  It locks the database directory with non-blocking advisory locks so it
can safely coexist with the utilities being replaced during migration.  A
commit writes and synchronizes a same-directory temporary file, links the old
database to `db.backup`, atomically renames the new state into place, and
synchronizes the directory.

Reference tools
---------------

`tools/pkginfo.cpp` provides a deliberately small reference inspector for the
public `libpkgstate` API.  It is useful for integration tests, format diagnosis,
and exercising ownership queries independently of `pkgman`.

```sh
build/tools/pkginfo -i
build/tools/pkginfo -l pkgname
build/tools/pkginfo -o /usr/bin/tool
build/tools/pkginfo -r /alternate/root -i
```

The tool reports durable installed state only.  It does not inspect package
archives or provide archive footprints; those are `libpkgimage` concerns.  It
is not intended to become a second package-management frontend.  `pkgman` is
the integration unit and user interface.

By default the executable remains in the build tree so it can coexist with the
historical `pkginfo` while migration is in progress.  Set
`-Dinstall_tools=true` only when installing the reference tools is desired.

The output contract is line-oriented: `-i` and `-o` print `name version`, while
`-l` prints canonical owned paths and retains the trailing slash on directory
entries.  An absent owner or package returns status 1; command-line misuse
returns status 2.

API documentation
-----------------

Public interfaces are documented with Doxygen comments under
`include/libpkgstate`.

Generate the HTML reference from the project root with:

```sh
doxygen Doxyfile
```

The output is written to `build/docs/html`.

Using the library
-----------------

```cpp
#include <libpkgstate/legacy_text_store.h>

pkgstate::legacy_text_store store("/var/lib/pkg/db");
pkgstate::snapshot state = store.read();

if (const auto* package = state.find_package("pkgutils"))
  std::cout << package->identity().version() << '\n';
```

Compiler and linker flags are available through pkg-config:

```sh
pkg-config --cflags --libs libpkgstate
pkg-config --static --libs libpkgstate
```

Layout
------

* `include/libpkgstate/` — public API;
* `src/` — library implementation;
* `tools/` — thin reference command-line clients;
* `tests/` — model, format, locking, transaction, and CLI tests; and
* `.github/workflows/` — shared/static CI builds.

License
-------

`libpkgstate` is licensed under the GNU General Public License version 3 or
later.  See `COPYING` for the license terms and `COPYRIGHT` for notices.
