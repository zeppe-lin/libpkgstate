libpkgstate design
==================

Purpose
-------

`libpkgstate` represents durable installed package state independently of the
archive, installation, audit, and orchestration layers.

Its input and output are facts about recorded ownership:

```text
package identity
ownership manifest
shared ownership
durable snapshot
```

The library does not decide what should be installed.  It records and queries
what a completed installation policy has declared installed.

Architectural boundary
----------------------

```text
archive truth                    installed-state truth
libpkgimage                      libpkgstate
     |                                |
     v                                v
package_image                    installed_package
     |                                |
     +---------- higher layer --------+
                    |
                    v
          policy and transaction
```

`libpkgimage` and `libpkgstate` intentionally describe different realities.

A package archive may contain an object that policy preserves, rejects, omits,
or replaces.  The resulting installed manifest therefore cannot be inferred
by copying the archive manifest into the state database.

`libpkgstate` does not:

* open package archives;
* replay package payloads;
* inspect the live filesystem;
* evaluate install or upgrade rules;
* create rejected objects;
* execute package scripts;
* resolve dependencies; or
* claim atomicity with filesystem changes.

Identity model
--------------

`package_identity` stores one package name and one version string.

Both fields are durable line-oriented data.  They must be:

* non-empty;
* free of NUL bytes;
* free of carriage returns; and
* free of newlines.

Naming policy beyond those representation constraints belongs to a higher
layer.  `libpkgstate` does not impose collection, repository, epoch, or release
syntax.

Ownership model
---------------

An `owned_entry` contains:

* one canonical `pkgimage::package_path`; and
* one durable object class.

The compatibility database preserves only two object classes:

```text
directory
non_directory
```

`non_directory` may represent a regular file, symbolic link, FIFO, socket, or
device.  The state model does not invent a more precise type than the durable
format carries.

An `installed_package` combines a validated identity with its ownership
manifest.

Construction:

* accepts manifest entries in arbitrary order;
* sorts entries by canonical path;
* rejects duplicate paths; and
* permits an empty manifest.

A manifest describes ownership after policy resolution.  It is not an archive
footprint and does not contain payload hashes, modes, owners, timestamps, link
targets, or device numbers.

Snapshot model
--------------

A `snapshot` is an immutable indexed collection of installed packages.

Construction:

* accepts packages in arbitrary order;
* sorts packages by name;
* rejects duplicate package names; and
* builds a reverse ownership index.

Queries provide:

* all packages in package-name order;
* exact package lookup;
* exact path ownership;
* shared ownership; and
* a boolean owned-path test.

Shared ownership is valid state.  It is retained and returned in deterministic
package-name order.  The model neither collapses it to one owner nor treats it
as an automatic error.

Storage model
-------------

`store` is the backend-neutral durable storage interface.

A read produces one immutable snapshot.  A write begins an isolated
`write_transaction` initialized from current durable state.

A write transaction supports:

* reading its uncommitted snapshot;
* adding or replacing a package by name;
* erasing a package by name;
* publishing the current state; and
* reporting whether publication completed.

Destroying an uncommitted transaction discards its uncommitted state.

The transaction boundary is deliberately narrow:

```text
state storage only
```

It does not coordinate payload staging, filesystem mutation, rejected-file
creation, post-install actions, or rollback.

Compatibility backend
---------------------

`legacy_text_store` implements the historical `/var/lib/pkg/db` format with
original Zeppe-Lin code.

It provides migration interoperability while keeping the storage format behind
the `store` interface.  Consumers should depend on `store`, `snapshot`, and
`write_transaction` when backend identity is not part of their contract.

Detailed parsing, locking, backup, rename, and synchronization semantics are
specified in `STORAGE.md` and `pkgstate_legacy_text_store(3)`.

Errors
------

All public failures derive from `pkgstate::error`:

```text
identity_error       invalid identity representation
state_error          invalid package or snapshot state
store_error          parsing, locking, I/O, or publication failure
transaction_error    invalid transaction lifecycle use
```

Errors carry human-readable diagnostics.  Consumers should select behavior
from the typed exception class rather than parse diagnostic text.

Reference tool
--------------

`pkginfo(1)` is a small composition client for `legacy_text_store` and the
`libpkgimage` libarchive backend.

It owns:

* command-line parsing;
* installed-package-first `-l` dispatch;
* alternate-root database selection;
* archive pathname inspection;
* line-oriented output;
* diagnostics; and
* exit-status policy.

The frontend may compose installed truth and archive truth because it does not
merge them.  Installed manifests remain `libpkgstate` facts; archive entries
remain `libpkgimage` facts.  The `libpkgstate` library still opens no archive.

The inherited `pkginfo -f` footprint command is not part of this composition.
Footprint generation and comparison belong to the build layer.  `pkgman`
remains the supported package-management integration layer.

Determinism
-----------

The following results are deterministic:

* installed-package order;
* per-package manifest order;
* owner-query order;
* compatibility database serialization; and
* `pkginfo(1)` line order.

Deterministic ordering is part of the public contract, not an incidental
property of one backend.

Concurrency
-----------

Snapshots are immutable values after construction.

Storage concurrency is backend-specific.  The compatibility backend uses
non-blocking advisory directory locks and is specified in `STORAGE.md`.

The public API does not claim that a snapshot automatically refreshes after
another process commits new state.  Consumers that require current state must
read a new snapshot.
