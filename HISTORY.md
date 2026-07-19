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
