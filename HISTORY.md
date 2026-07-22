libpkgstate history
===================

Lineage
-------

`libpkgstate` is original Zeppe-Lin code.

It was created to replace the package-database and installed-ownership concerns
formerly entangled with CRUX-derived tooling.  No CRUX `pkgutils` or
`libpkgcore` implementation was imported.

0.1.0
-----

The first release established:

* validated package identities;
* canonical installed ownership manifests;
* immutable indexed snapshots;
* explicit shared ownership;
* backend-neutral stores and write transactions;
* the original `/var/lib/pkg/db` compatibility backend;
* non-blocking directory locking;
* synchronized complete-state publication; and
* model, parser, transaction, and locking tests.

0.2.0
-----

The second release added:

* the reference `pkginfo(1)` inspector;
* exact package and owner queries;
* deterministic line-oriented output;
* alternate-root support;
* CLI status contracts; and
* black-box command tests.

Documentation reconstruction
----------------------------

The documentation reconstruction added:

* explicit architecture and storage contracts;
* migration guidance for replacing inherited `pkginfo`;
* a testing doctrine;
* library, model, store, backend, format, and command manual pages;
* independent public-header tests;
* documentation consistency tests; and
* CI gates for scdoc and Doxygen.

0.3.0
-----

The third release adds archive listing to the reference frontend without
changing the installed-state library boundary:

* `pkginfo -l` accepts an installed package or a regular package archive;
* installed package identity takes precedence over same-named files;
* archive entries are inspected through `libpkgimage` 0.3.0 and retain archive
  order;
* directory suffixes remain explicit in both installed and archive output;
* malformed and non-regular archive operands have defined failures;
* migration documentation records `pkginfo -f` as the remaining build-layer
  transition gate; and
* the public libpkgstate ABI and soversion remain unchanged.

Canonical generation storage
----------------------------

The canonical authority reconstruction added the first lossless concrete state
backend:

* one durable target binding per store directory;
* complete binary canonical snapshot generations;
* content-addressed immutable generation names;
* non-blocking shared reads and exclusive compare-and-publish locking;
* atomic current-generation selection;
* file and directory synchronization with truthful durability outcomes;
* post-selection authoritative reread;
* recomputation of every state-owned identity while loading; and
* explicit separation between current-state authority and receipt journaling.

The historical `/var/lib/pkg/db` backend remains byte-compatible and separate.
