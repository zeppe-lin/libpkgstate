Migrating pkginfo to libpkgstate
================================

Purpose
-------

The `libpkgstate` reference `pkginfo(1)` replaces the executable inherited
through `pkgutils` without inheriting its implementation.

The migration preserves the useful inspection surface while making durable
installed state the explicit source of truth.

Package transition
------------------

During the packaging transition:

1. remove `pkginfo` from the `pkgutils` package footprint;
2. install `pkginfo` from the `libpkgstate` package or a dedicated package;
3. retain `/var/lib/pkg/db` as the compatibility state source; and
4. avoid installing both executables at the same pathname.

The new tool is built by the `tools` option and installed only when
`install_tools=true`.

Preserved commands
------------------

The replacement provides:

```text
pkginfo -i
pkginfo -l package
pkginfo -o path
pkginfo -r root ...
pkginfo -V
pkginfo -h
```

Long options are documented in `pkginfo(1)`.

Defined output
--------------

The replacement defines deterministic line-oriented output:

```text
-i    name version, one package per line
-l    canonical root-relative owned paths
-o    name version, one exact owner per line
```

Directory entries printed by `-l` retain a trailing slash.

Shared ownership is not hidden.  `-o` prints every owner in package-name
order.

Defined status
--------------

```text
0    query completed successfully
1    package or owner absent, or runtime failure
2    command-line misuse
```

Diagnostics are written to standard error.

Behavior deliberately not provided
-----------------------------------

The replacement reports durable installed state only.

It does not:

* inspect package archives;
* print archive footprints;
* guess owners through regular expressions;
* collapse shared ownership;
* mutate package state; or
* act as a package-management orchestrator.

Archive contents belong to `libpkgimage`.  Package-management integration
belongs to `pkgman`.

Database compatibility
----------------------

`legacy_text_store` is an original implementation of the existing
line-oriented `/var/lib/pkg/db` format.

The transition therefore does not require a database conversion.  Existing
valid package records are read into the new canonical model.

Invalid historical records are rejected rather than silently normalized beyond
the documented path and ordering rules.

Operational check
-----------------

Before removing the inherited executable, compare the replacement against the
installed database:

```sh
new-pkginfo -i
new-pkginfo -l pkgutils
new-pkginfo -o /usr/bin/pkgadd
new-pkginfo -r /alternate/root -i
```

Then verify:

* package order;
* canonical path spelling;
* directory suffixes;
* shared owners;
* absent-package status;
* absent-owner status; and
* alternate-root database selection.
