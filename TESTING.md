libpkgstate testing
===================

Doctrine
--------

Durable state code is tested as a contract boundary.

The suite must establish:

* model validation;
* canonical ordering;
* duplicate rejection;
* shared ownership;
* parser strictness;
* deterministic serialization;
* transaction lifecycle;
* lock interoperability;
* publication failure behavior;
* reference-tool output and status;
* independent public-header usability; and
* documentation consistency.

Canonical model tests
---------------------

Canonical model tests cover:

* strict algorithm-qualified digest parsing and domain separation;
* state-owned package-path normalization and conformance vectors against
  `libpkgimage` without a core dependency;
* package release, installed-control, and target-binding identity vectors;
* known-empty versus historically unavailable control groups;
* complete installed-package construction and identity recomputation;
* snapshot target consistency, canonical package order, and duplicate rejection;
* ownership-inventory and snapshot identity vectors;
* exact package lookup, exact ownership lookup, and deterministic shared owners;
* immutable publication requests, normalized deltas, and evidence constraints;
* typed receipt outcome, durability, atomicity, and result invariants; and
* permutation and change-sensitivity checks for every canonical identity.

Compatibility and migration model tests
---------------------------------------

Compatibility and migration tests cover:

* fixed retained, derived, and historically unavailable fact profiles;
* known-empty legacy manifests;
* package and snapshot observation identities;
* explicit release decomposition and unavailable-fact decisions;
* exact source-observation and migration-evidence binding;
* dry-run validated receipts;
* fresh-destination import and stale-destination refusal; and
* preservation of legacy source provenance in canonical installed control.

Storage tests
-------------

Canonical generation tests cover:

* empty initialization and interrupted-initialization recovery;
* binding and snapshot fixed storage vectors;
* complete control and ownership round trips;
* immutable generation and selector permissions;
* selector authority and orphan-generation exclusion;
* exact decode, identity recomputation, and canonical re-encoding;
* stale compare-and-publish without backend mutation;
* install, replacement, removal, and generation reuse;
* published, durability-unconfirmed, and indeterminate outcomes;
* shared and exclusive lock interoperability;
* corruption, hard-link, writable-object, and binding-mismatch rejection; and
* non-initializing `open_existing()` behavior.

Compatibility-format and transaction tests cover:

* complete records;
* empty manifests;
* directory suffixes;
* final records ending at end-of-file;
* malformed identities;
* malformed paths;
* duplicate package names;
* duplicate owned paths;
* unreadable or absent storage; and
* deterministic byte-compatible round trips;
* package insertion, replacement, and erasure;
* invalid erase names and transaction lifecycle rejection;
* uncommitted destruction, mode `0444`, and backup replacement;
* same-directory publication; and
* rereading the committed snapshot.

Locking tests
-------------

Locking and publication tests cover:

* concurrent compatibility readers and writer exclusion;
* canonical shared readers and exclusive publication exclusion;
* comparison and authoritative reread under one publication lock;
* refusal to invoke backend publication for stale state;
* lock release on destruction, failure, and successful publication; and
* interoperability with external processes using the documented lock domains.

Reference-tool tests
--------------------

Black-box `pkginfo(1)` tests cover:

* installed package listing;
* installed package manifest listing;
* package archive listing;
* archive order and directory suffixes;
* installed-package precedence over a same-named file;
* malformed, missing, and non-regular archive operands;
* empty installed manifests;
* exact owner lookup;
* shared ownership;
* alternate roots;
* unowned paths;
* absent packages;
* conflicting query modes;
* version output;
* standard-output versus standard-error separation; and
* statuses 0, 1, and 2.

Black-box `pkgstate-check(1)` tests cover:

* canonical binding and snapshot diagnostics;
* canonical control-completeness counts;
* compatibility observation and fact-availability diagnostics;
* shared-ownership counts;
* rejection of a mismatched canonical target binding;
* refusal to initialize absent or incomplete canonical storage;
* unchanged canonical and compatibility file contents;
* all-or-nothing standard output on failure;
* help and version output; and
* statuses 0, 1, and 2.

Planner-adapter tests
---------------------

The optional adapter tests cover:

* verbatim transfer through matching strong identity domains;
* canonical path reparsing and complete ownership projection;
* package, claim, and shared-owner ordering;
* target-binding mismatch rejection;
* absent filesystem metadata remaining absent;
* no `legacy_snapshot` projection path; and
* separate core and adapter pkg-config dependency closure.

Public-header tests
-------------------

Every installed public header is compiled independently.

This catches:

* missing direct includes;
* accidental include-order dependencies;
* undeclared public dependency requirements; and
* umbrella-header-only success that hides broken individual headers.

Documentation tests
-------------------

The documentation contract test checks:

* presence of every documented manual source;
* page names and sections;
* authority-graph and dependency-direction coverage;
* request, stale-state, receipt, and atomicity coverage;
* `pkginfo(1)` option, dispatch, ordering, and status coverage;
* the explicit absence and migration boundary of `pkginfo -f`;
* state-versus-filesystem transaction warnings;
* compatibility database record grammar;
* locking and backup documentation;
* consistency between `pkginfo --help` and the manual page; and
* the read-only and completeness contracts of `pkgstate-check(1)`.

Build matrix
------------

CI qualifies the canonical and compatibility paths through one forge-neutral
script surface under `ci/`. The normal matrix runs:

* GCC and Clang;
* separate shared and static dependency stacks;
* the optional `libpkgstate-plan` adapter;
* `pkginfo(1)` compatibility and archive composition;
* `pkgstate-check(1)` canonical and legacy diagnostics;
* warnings as errors;
* the complete test suite;
* installed public-header isolation;
* installed pkg-config consumers; and
* linkage-closure checks that keep `libpkgimage` and `libpkgplan` out of the
  core library.

The GCC shared leg additionally generates and lints every manual with `mandoc`
and builds the Doxygen reference with warnings as errors. Separate jobs qualify
an optimized `NDEBUG` build and GCC/Clang address and undefined-behavior
sanitizers.

The workflow pins the released `libpkgimage` 0.3.0 and `libpkgplan` 0.1.0
commits, builds them into an isolated prefix, and configures `libpkgstate` with
`--wrap-mode=nofallback`. This prevents an unrelated host installation or
implicit fallback from changing the tested authority graph.

Run the same entry points locally after placing those dependency sources at
`subprojects/libpkgimage` and `subprojects/libpkgplan`:

```sh
CC=gcc CXX=g++ \
  ./ci/configure-and-test.sh build-ci shared \
    --buildtype=debug -Dman_pages=enabled
./ci/lint-manpages.sh build-ci
CXX=g++ ./ci/qualify-installed.sh build-ci shared
```

A release should not proceed while any contract test is disabled merely to
obtain a green build.

Adding behavior
---------------

Every new model invariant, storage branch, error class, command option, or
published output rule requires:

1. a direct positive test;
2. a direct negative or boundary test;
3. documentation in the relevant contract document;
4. manual-page coverage when user-visible; and
5. deterministic-order tests where ordering is observable.
