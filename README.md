libpkgstate
===========

`libpkgstate` is the Zeppe-Lin C++17 library for representing and persisting
durable installed package state.

It provides:

* strongly typed algorithm-qualified state identities;
* canonical package releases with computed identities;
* immutable durable installed control with explicit completeness;
* durable target-state bindings with computed identities;
* complete canonical installed package records with computed identities;
* complete immutable snapshots with ownership and snapshot identities;
* immutable expected-snapshot publication requests and package deltas;
* typed immutable publication receipts with computed identities;
* non-overridable canonical compare-and-publish orchestration;
* an immutable-generation canonical state backend;
* state-owned canonical package paths and explicit shared ownership;
* explicitly incomplete compatibility records and snapshots;
* explicit receipt-bound import into a fresh canonical store;
* an optional non-contaminating projection adapter for `libpkgplan`;
* non-mutating diagnostics for canonical and compatibility state;
* compatibility write transactions for the historical database;
* an original compatibility backend for `/var/lib/pkg/db`.

The central distinction is:

```text
pkgimage::package_image       what an archive contains
pkgstate::installed_package   complete durable installed truth
```

An installed manifest is the ownership state that remains after completed
application policy.  It is not a copy of an archive manifest.

The historical database is deliberately represented by different public types:

```text
legacy_installed_package      name + opaque version line + ownership
legacy_snapshot               one incomplete database observation
```

Those values do not pretend to contain canonical releases, installed control,
target binding, typed installed identities, or canonical snapshot identity.
They expose fixed completeness profiles and separate observation identities for
the exact incomplete facts that were read.

The `libpkgstate` library does not inspect package archives, apply installation
policy, mutate the installed filesystem, stage rejected objects, run maintenance
actions, or coordinate a complete package transaction.  The optional `pkginfo`
frontend composes compatibility-state queries with `libpkgimage` archive
inspection.  The separate `pkgstate-check` frontend validates canonical or
compatibility state without moving, repairing, importing, or publishing either
authority.

The implementation is original Zeppe-Lin code.  It is not derived from CRUX
`pkgutils` or from the former CRUX-derived `libpkgcore`.

Contracts
---------

The canonical model is:

```text
package_release --------> installed_control
       |                         |
       +------------+------------+
                    |
state_target_binding+ completed ownership manifest
                    |
                    v
           installed_package
             computed identity
                    |
                    v
          ownership inventory
             computed identity
                    |
                    v
               snapshot
       one target binding, package lookup,
       derived shared ownership index,
       and computed snapshot identity
```

`installed_package::make()` requires one canonical release, control for exactly
that release, one target binding, and the completed ownership manifest.  The
library normalizes the manifest and computes `installed_package_identity`
from those facts.

`snapshot::make()` accepts only complete canonical installed packages.  Every
package must carry the snapshot's exact target binding.  Packages are sorted by
name, duplicate package names are rejected, and shared path ownership remains
valid state.  The library computes `ownership_inventory_identity` from the
normalized path-to-installed-package relation, then computes
`installed_state_snapshot_identity` from the schema version, target binding,
installed-package identities, and ownership-inventory identity.

`state_publication_request::make()` binds one exact expected snapshot to one or
more non-conflicting package deltas.  Install and replacement deltas carry
complete proposed installed packages; replacement and removal deltas require
the exact old installed-package identity.  Each delta cites one accepted
operation plan and matching completed application evidence.  Multi-package
requests require transaction evidence.  The request is identified by
libpkgstate and is not a caller-authored replacement snapshot.

`state_publication_receipt` classifies one actual publication attempt.  Its
factories bind the request to the actual prior snapshot, target and store
binding, backend storage format, typed outcome, durability knowledge, actual
state-storage atomicity boundary, optional resulting snapshot, and normalized
subordinate evidence.  Successful factories validate the exact resulting
snapshot implied by the request deltas.  A stale-state receipt requires a
different actual prior snapshot and records no mutation.

`canonical_store::compare_and_publish()` owns the stale-safe publication
algorithm. It begins one exclusive backend transaction, obtains the
actual durable snapshot under that lock, compares its identity with the
request, refuses stale state without calling publication, derives the
only permitted resulting snapshot, and maps the constrained backend
result into a typed receipt. The method is non-virtual; a backend cannot
override away the comparison.

`canonical_generation_store` is the concrete lossless backend. It durably
binds one store directory to one target binding, writes complete snapshots as
immutable content-addressed generations, atomically selects one generation as
current, synchronizes the selection domain, and rereads the result under the
publication lock. Initialization creates and selects the empty snapshot
generation, so a successfully opened store always has one authoritative current
selector.

Compatibility storage uses a separate model:

```text
package_identity                 historical name + opaque version line
legacy_installed_package         compatibility ownership record
legacy_package_completeness      retained, derived, and unavailable facts
legacy_snapshot                  indexed compatibility observation
legacy_snapshot_completeness     snapshot-level fact availability
legacy observation identities    exact incomplete source observations
store / write_transaction        compatibility mutation only
legacy_text_store                /var/lib/pkg/db backend
```

A compatibility read does not parse an opaque version line into canonical
version and release fields.  It does not reconstruct missing control or target
facts from current sources, archives, filenames, configuration, or the live
filesystem.  Completeness profiles distinguish facts retained by the database
format, facts derived only from those retained values, and facts that are
historically unavailable.  Package and snapshot observation identities name
the exact incomplete source without impersonating canonical installed
authority.

`legacy_import_request::make()` binds one exact compatibility observation to a
fresh empty canonical destination. Every source package requires one explicit
release decomposition, explicit supplied-or-unavailable control facts, and one
decision for every external provenance class. Construction computes the only
admissible canonical snapshot without mutating storage.

`canonical_store::import_legacy()` compares the empty destination under the
backend publication lock and publishes only that precomputed result. It returns
a typed `legacy_import_receipt`; stale destination state never invokes the
backend publication primitive. Reading `legacy_text_store` alone is not
migration.

The optional `libpkgstate-plan` adapter projects one complete canonical
`snapshot` into planner-owned installed-package and ownership facts. It copies
only canonical algorithm-qualified identities and normalized package paths. The
caller supplies a complete `target_system_context_identity` together with its
durable `state_target_binding` projection; the adapter refuses a snapshot whose
binding differs. The adapter accepts no `legacy_snapshot`, invents no target
context, and supplies no filesystem metadata absent from planner vocabulary.

Important invariants:

* canonical release coordinates are non-empty and line-safe;
* installed control distinguishes known empty facts from historical absence;
* unavailable control groups cannot contain invented values;
* target bindings contain typed identities, not root pathnames;
* installed packages bind release, control, target, and completed ownership;
* installed-package identity excludes containing snapshot identity;
* canonical snapshots contain packages from exactly one target binding;
* ownership and snapshot identities are computed from normalized state;
* publication requests compare against one exact prior snapshot identity;
* proposed records retain the application and transaction evidence they cite;
* composed requests contain no conflicting package-name deltas;
* receipts distinguish stale, rejected, failed, published, durability-unknown,
  and indeterminate publication outcomes;
* successful receipts validate the request-implied resulting snapshot;
* canonical compare-and-publish performs the comparison under one lock;
* stale requests never invoke the backend publication primitive;
* one current selector is the sole authority among immutable generations;
* selected generation identity is recomputed from complete decoded state;
* package paths are canonical and root-relative;
* package manifests contain at most one entry for each path;
* packages, manifests, and owner results have deterministic order;
* multiple packages may own the same path;
* canonical and compatibility snapshots are immutable after construction; and
* compatibility transactions mutate compatibility storage only;
* migration requires one exact source observation and a fresh destination;
* every unavailable migration fact is declared explicitly; and
* legacy import never parses version lines or reconstructs current sources.

A `canonical_publication_transaction` is the mechanism half of canonical
publication. The concrete `canonical_generation_store` holds one non-blocking
exclusive directory lock across comparison, generation installation, selection,
durability synchronization, and result reread. It does not own the comparison.

A `write_transaction` is not a canonical publication request and is not a
filesystem transaction.  It accepts `legacy_installed_package` values only.
Publishing compatibility state does not make preceding installation or removal
operations atomic and does not provide expected-snapshot comparison.

The compatibility backend reads and writes the documented line-oriented
`/var/lib/pkg/db` format.  Reads hold a non-blocking shared lock on the database
directory.  Writes hold a non-blocking exclusive lock from `begin_write()` until
commit or destruction.

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
* `pkgstate_authority(7)` — authority graph and dependency direction;
* `pkgstate_model(3)` — canonical installed facts, snapshots, and publication
  requests;
* `pkgstate_publication(3)` — compare-and-publish requests, outcomes, and
  receipts;
* `pkgstate_store(3)` — canonical and compatibility store interfaces;
* `pkgstate_canonical_generation_store(3)` — immutable-generation backend;
* `pkgstate_legacy_compatibility(3)` — incomplete facts and observation
  identities;
* `pkgstate_legacy_import(3)` — explicit validation and canonical import;
* `pkgstate_plan_adapter(3)` — optional projection into `libpkgplan`;
* `pkgstate_legacy_text_store(3)` — compatibility backend;
* `pkgstate-db(5)` — line-oriented compatibility database format;
* `pkgstate-generation(5)` — canonical generation storage format;
* `pkginfo(1)` — optional compatibility and archive inspector; and
* `pkgstate-check(1)` — optional read-only state diagnostics.

The reference commands are installed only when `-Dinstall_tools=true` is
selected.

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

The optional planner adapter requires:

* `libpkgplan` 0.1.0 or later when `-Dplanner_adapter=enabled` is selected.

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
meson setup build-plan -Dplanner_adapter=enabled
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

Read-only state diagnostics
---------------------------

`pkgstate-check(1)` validates an existing canonical generation store or one
historical compatibility database without changing either source:

```sh
build/tools/pkgstate-check --legacy-database /var/lib/pkg/db

build/tools/pkgstate-check \
  --canonical-store /var/lib/pkg/state \
  --managed-target v1:sha256:<digest> \
  --state-store v1:sha256:<digest> \
  --root-view v1:sha256:<digest> \
  --state-backend v1:sha256:<digest> \
  --publication-domain v1:sha256:<digest>
```

Canonical mode requires the caller's exact target-binding components and opens
only a complete existing store through
`canonical_generation_store::open_existing()`.  It never initializes storage
or completes interrupted initialization.  Compatibility mode reports the exact
legacy observation identity and fixed retained, derived, and historically
unavailable fact classes without parsing opaque version lines or importing
state.

Successful output is deterministic `key=value` presentation.  It is not a new
state-authority protocol and must not be parsed as a replacement for the typed
library API.

Using the library
-----------------

```cpp
#include <iostream>

#include <libpkgstate/legacy_text_store.h>

pkgstate::legacy_text_store store("/var/lib/pkg/db");
const pkgstate::legacy_snapshot state = store.read();

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

* `include/libpkgstate/` — installed-state public API;
* `include/libpkgstate-plan/` — optional planner adapter API;
* `adapter/` — optional cross-library projection implementation;
* `src/` — library implementation;
* `tools/` — optional reference clients;
* `tests/` — model, storage, locking, CLI, header, and documentation tests;
* `man/` — scdoc manual sources; and
* `.github/workflows/` — build and documentation gates.

License
-------

`libpkgstate` is licensed under the GNU General Public License version 3 or
later.  See `COPYING` for the license terms and `COPYRIGHT` for notices.
