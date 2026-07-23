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

Model tests
-----------

Model tests cover:

* valid and invalid package identities;
* path canonicalization delegated to `libpkgimage`;
* arbitrary input ordering;
* sorted installed manifests;
* duplicate manifest paths;
* empty manifests;
* sorted package snapshots;
* duplicate package names;
* exact package lookup;
* exact ownership lookup; and
* multiple owners in deterministic order.

Storage tests
-------------

Compatibility-format tests cover:

* complete records;
* empty manifests;
* directory suffixes;
* final records ending at end-of-file;
* malformed identities;
* malformed paths;
* duplicate package names;
* duplicate owned paths;
* unreadable or absent storage; and
* deterministic round trips.

Transaction tests cover:

* initial state capture;
* package insertion;
* package replacement;
* package erasure;
* invalid erase names;
* uncommitted destruction;
* one successful commit;
* rejection of post-commit mutation;
* mode `0444`;
* backup replacement;
* same-directory publication; and
* rereading the committed snapshot.

Locking tests
-------------

The compatibility backend uses non-blocking advisory directory locks.

Tests cover:

* concurrent shared readers;
* writer exclusion;
* reader exclusion while a writer is open;
* lock release on transaction destruction;
* lock release after commit; and
* interoperability with processes using the same directory-lock contract.

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

CI runs:

* shared library and shared dependencies;
* static library and static dependencies;
* warnings as errors;
* the complete test suite;
* scdoc manual generation;
* Doxygen with warnings as errors; and
* staged installation.

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
