libpkgstate
===========

`libpkgstate` is the Zeppe-Lin C++17 library for representing and persisting
durable installed package state.

It provides:

* validated package names and versions;
* canonical installed ownership manifests;
* immutable snapshots with package and ownership indexes;
* explicit shared ownership;
* backend-neutral state write transactions; and
* an original compatibility backend for `/var/lib/pkg/db`.

The central distinction is:

```text
pkgimage::package_image       what an archive contains
pkgstate::installed_package   what durable state says is installed
```

An installed manifest is the ownership state that remains after installation
policy has been resolved.  It is not a copy of an archive manifest.

`libpkgstate` does not inspect package archives, apply installation policy,
mutate the installed filesystem, stage rejected objects, run maintenance
actions, or coordinate a complete package transaction.

The implementation is original Zeppe-Lin code.  It is not derived from CRUX
`pkgutils` or from the former CRUX-derived `libpkgcore`.

Contracts
---------

The public model has four levels:

```text
package_identity
       |
       v
installed_package ----> ordered owned_entry manifest
       |
       v
snapshot -------------> package lookup and shared ownership
       |
       v
store / write_transaction
```

Important invariants:

* package identity fields are non-empty and line-safe;
* package paths are canonical and root-relative;
* an installed package contains at most one entry for each path;
* packages and manifests are returned in deterministic order;
* multiple packages may own the same path;
* snapshots are immutable after construction; and
* state transactions mutate storage only.

A `write_transaction` is not a filesystem transaction.  Publishing package
state does not make preceding installation or removal operations atomic.

The compatibility backend reads and writes the documented line-oriented
`/var/lib/pkg/db` format.  Reads hold a non-blocking shared lock on the
database directory.  Writes hold a non-blocking exclusive lock from
`begin_write()` until commit or destruction.

See:

* `DESIGN.md` for the architecture and invariants;
* `STORAGE.md` for store, locking, and publication semantics;
* `MIGRATION.md` for replacing the inherited `pkginfo(1)`;
* `TESTING.md` for the verification doctrine; and
* `HISTORY.md` for project lineage.

Manual pages
------------

The installed manual suite is:

* `libpkgstate(3)` — library overview and boundaries;
* `pkgstate_model(3)` — identities, ownership manifests, and snapshots;
* `pkgstate_store(3)` — stores and write transactions;
* `pkgstate_legacy_text_store(3)` — compatibility backend;
* `pkgstate-db(5)` — line-oriented database format; and
* `pkginfo(1)` — optional reference inspector.

`pkginfo(1)` is installed only when `-Dinstall_tools=true` is selected.

Requirements
------------

Build-time requirements:

* a C++17 compiler;
* Meson 1.2.0 or later;
* Ninja;
* pkg-config; and
* `libpkgimage` 0.2.1 or later.

Optional documentation dependencies:

* `scdoc` for manual pages; and
* Doxygen for the generated API reference.

Building
--------

Shared library:

```sh
meson setup build
meson compile -C build
meson test -C build --print-errorlogs
```

Static library with static dependencies:

```sh
meson setup build-static \
  -Ddefault_library=static \
  -Dlink_mode=static
meson compile -C build-static
meson test -C build-static --print-errorlogs
```

The project rejects `default_library=both`.  Shared and static artifacts are
separate builds, and each build links the corresponding form of its
dependencies.

Useful options:

```sh
meson setup build -Dtests=disabled
meson setup build -Dtools=disabled
meson setup build -Dman_pages=disabled
meson setup build-tools -Dinstall_tools=true
```

Manual pages use `man_pages=auto` by default and are built when `scdoc` is
available.

Installation:

```sh
meson install -C build
```

Reference pkginfo
-----------------

The optional `tools/pkginfo.cpp` executable is a small inspector for durable
installed state:

```sh
build/tools/pkginfo -i
build/tools/pkginfo -l pkgname
build/tools/pkginfo -o /usr/bin/tool
build/tools/pkginfo -r /alternate/root -i
```

It reports installed-state facts only.  It does not inspect package archives
or print archive footprints.  `pkgman` remains the package-management
integration unit.

The output contract is line-oriented:

* `-i` and `-o` print `name version`;
* `-l` prints canonical root-relative owned paths;
* directory ownership retains a trailing slash;
* status 1 reports an absent package, absent owner, or runtime failure; and
* status 2 reports command-line misuse.

Using the library
-----------------

```cpp
#include <iostream>

#include <libpkgstate/legacy_text_store.h>

pkgstate::legacy_text_store store("/var/lib/pkg/db");
const pkgstate::snapshot state = store.read();

if (const auto* package = state.find_package("pkgutils"))
  std::cout << package->identity().version() << '\n';
```

Compiler and linker flags are available through pkg-config:

```sh
pkg-config --cflags --libs libpkgstate
pkg-config --static --libs libpkgstate
```

API documentation
-----------------

Public interfaces are documented with Doxygen comments under
`include/libpkgstate`.

```sh
doxygen Doxyfile
```

Generated HTML is written to `build/docs/html`.

Layout
------

* `include/libpkgstate/` — installed public API;
* `src/` — library implementation;
* `tools/` — optional reference clients;
* `tests/` — model, storage, locking, CLI, header, and documentation tests;
* `man/` — scdoc manual sources; and
* `.github/workflows/` — build and documentation gates.

License
-------

`libpkgstate` is licensed under the GNU General Public License version 3 or
later.  See `COPYING` for the license terms and `COPYRIGHT` for notices.
