Migrating pkginfo to libpkgstate
================================

Purpose
-------

The `libpkgstate` reference `pkginfo(1)` replaces the installed-state and
archive-listing behavior of the executable inherited through `pkgutils`
without inheriting its implementation.

The frontend composes two explicit sources of truth:

```text
installed package and ownership queries    libpkgstate
package archive content listing            libpkgimage
```

It does not collapse an archive image into installed ownership state.

Preserved commands
------------------

The replacement provides:

```text
pkginfo -i
pkginfo -l package
pkginfo -l package-archive
pkginfo -o path
pkginfo -r root ...
pkginfo -V
pkginfo -h
```

Long options are documented in `pkginfo(1)`.

List dispatch
-------------

`pkginfo -l argument` applies the following order:

1. query the selected `legacy_snapshot` for package name `argument`;
2. when no installed package matches, require `argument` to name a regular
   file; and
3. inspect that file as a package archive through `libpkgimage`.

Installed package identity therefore takes precedence over a same-named file.
The `-r` option selects the installed database only; it does not reinterpret an
archive pathname beneath the alternate root.

Defined output
--------------

The replacement defines line-oriented output:

```text
-i    name version, one package per line
-l    canonical root-relative paths, one entry per line
-o    name version, one exact owner per line
```

Installed manifests are printed in canonical path order.  Archive entries are
printed in archive order.  Directory entries retain a trailing slash.

Shared ownership is not hidden.  `-o` prints every owner in package-name order.

Defined status
--------------

```text
0    query completed successfully
1    operand or owner absent, invalid archive, or runtime failure
2    command-line misuse
```

Diagnostics are written to standard error.

Footprint transition gate
-------------------------

The replacement does not provide the inherited:

```text
pkginfo -f package-archive
```

That command renders build-footprint data.  It belongs to the build layer, not
to installed state and not to raw archive truth alone.  `libpkgimage` provides
the archive image; footprint conversion and policy belong to `libpkgbuild` and
its reference frontend integration.

Historical CRUX `pkgmk` calls `pkginfo -f` while generating or checking a
footprint.  A system still using that `pkgmk` must retain the inherited
`pkginfo` executable.  Do not remove it merely because the new reference tool
covers `-i`, `-l`, and `-o`.

Full pathname replacement becomes valid only after one of these transitions:

* `pkgmk` is replaced by a build frontend that owns footprint rendering; or
* a compatible `pkginfo -f` frontend is supplied by the build layer.

Package transition
------------------

After the footprint transition gate is closed:

1. remove the inherited `pkginfo` from the `pkgutils` package footprint;
2. install the reference tool from `libpkgstate` or a dedicated package;
3. retain `/var/lib/pkg/db` as the compatibility state source; and
4. avoid installing both executables at the same pathname.

The new tool is built by the `tools` option and installed only when
`install_tools=true`.

Behavior deliberately not provided
-----------------------------------

The replacement does not:

* render or compare build footprints;
* guess owners through regular expressions;
* collapse shared ownership;
* mutate package state;
* apply archive payloads; or
* act as a package-management orchestrator.

Database compatibility
----------------------

`legacy_text_store` is an original implementation of the existing
line-oriented `/var/lib/pkg/db` format.

The frontend transition therefore does not require a database conversion.
Existing valid records are read into `legacy_installed_package` values inside a
`legacy_snapshot`.

That compatibility observation is not a canonical installed-state snapshot. It
has no canonical release decomposition, installed control, target binding,
installed-package identity, ownership-inventory identity, or canonical snapshot
identity.  Its completeness profile reports those facts as historically
unavailable.  Separate package and snapshot observation identities commit to the
exact incomplete facts that were read, so a later migration can cite its source
without promoting it to canonical installed authority.  The frontend needs none
of the unavailable facts for its preserved `-i`, `-l`, and `-o` queries.

Invalid historical records are rejected rather than silently normalized beyond
the documented path and ordering rules. Missing canonical facts are not
reconstructed from current sources, archives, filenames, or configuration.

Operational check
-----------------

Before replacing the inherited executable, compare the reference tool against
the installed database and real package archives:

```sh
new-pkginfo -i
new-pkginfo -l pkgutils
new-pkginfo -l /var/cache/pkgmk/packages/pkgutils#version-release.pkg.tar.xz
new-pkginfo -o /usr/bin/pkgadd
new-pkginfo -r /alternate/root -i
old-pkginfo -f /var/cache/pkgmk/packages/pkgutils#version-release.pkg.tar.xz
```

Then verify:

* package order;
* installed manifest order;
* archive order;
* directory suffixes;
* shared owners;
* installed-package precedence over a same-named file;
* malformed archive status;
* absent-operand and absent-owner status;
* alternate-root database selection; and
* the remaining `pkginfo -f` caller inventory.
