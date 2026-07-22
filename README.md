libpkgstate
===========

`libpkgstate` is the Zeppe-Lin C++17 library for representing and persisting
durable installed package state.

It provides:

* validated package names and versions;
* strongly typed algorithm-qualified state identities;
* canonical package releases with computed identities;
* immutable durable installed control with explicit completeness;
* durable target-state bindings with computed identities;
* state-owned canonical package paths;
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

The `libpkgstate` library does not inspect package archives, apply installation
policy, mutate the installed filesystem, stage rejected objects, run maintenance
actions, or coordinate a complete package transaction.  The optional `pkginfo`
reference frontend composes installed-state queries with `libpkgimage` archive
inspection without moving either authority into the other library.

The implementation is original Zeppe-Lin code.  It is not derived from CRUX
`pkgutils` or from the former CRUX-derived `libpkgcore`.

Contracts
---------

The canonical release fact and compatibility installed model currently coexist:

```text
package_release --------> installed_control
    coordinates            runtime dependencies
    computed identity      removal lifecycle material
                           target/profile facts
                           retained provenance
                           computed identity

managed target + store + root view + backend + publication domain
                              |
                              v
                    state_target_binding
                       computed identity

package_identity      package_path
       |                    |
       +---------+----------+
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

Canonical state identities use distinct C++ types and the strict textual form
`v1:sha256:<lowercase-hex>`.  `package_release` and `installed_control` use that
substrate now; `state_target_binding` uses it for the durable storage and
publication projection.  The compatibility-shaped installed package and
snapshot acquire canonical identities in later model commits.

Important invariants:

* canonical release coordinates are non-empty and line-safe;
* installed control distinguishes known empty facts from historical absence;
* unavailable control groups cannot contain invented values;
* target bindings contain typed identities, not root pathnames;
* target bindings exclude installed-snapshot identity;
* legacy package identity fields are non-empty and line-safe;
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
* `pkgstate_model(3)` — package releases, installed control, target binding,
  ownership, and snapshots;
* `pkgstate_store(3)` — stores and write transactions;
* `pkgstate_legacy_text_store(3)` — compatibility backend;
* `pkgstate-db(5)` — line-oriented database format; and
* `pkginfo(1)` — optional reference inspector.

`pkginfo(1)` is installed only when `-Dinstall_tools=true` is selected.

Requirements
------------

Build-time requirements:

* a C++17 compiler;
* Meson 1.6.0 or later;
* Ninja;
* pkg-config; and
* `libcrypto` with SHA-256 EVP support.

The core library has no `libpkgimage` dependency.  The optional `pkginfo`
frontend requires:

* `libpkgimage` 0.3.0 or later when reference tools are enabled.

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

The optional `tools/pkginfo.cpp` executable is a small composition client for
installed state and package archive contents:

```sh
build/tools/pkginfo -i
build/tools/pkginfo -l pkgname
build/tools/pkginfo -l package.pkg.tar.xz
build/tools/pkginfo -o /usr/bin/tool
build/tools/pkginfo -r /alternate/root -i
```

For `-l`, an installed package name takes precedence over a same-named archive
path.  Installed manifests come from `libpkgstate`; archive images come from
`libpkgimage`.  The library boundary remains intact.

The output contract is line-oriented:

* `-i` and `-o` print `name version`;
* installed `-l` results use canonical path order;
* archive `-l` results preserve archive order;
* directories retain a trailing slash;
* status 1 reports an absent operand, absent owner, invalid archive, or runtime
  failure; and
* status 2 reports command-line misuse.

The reference tool does not implement the inherited `pkginfo -f` footprint
command.  Existing `pkgmk` deployments that call `pkginfo -f` must retain the
old frontend until the build layer provides that command.  `pkgman` remains the
package-management integration unit.

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
