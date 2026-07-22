libpkgstate design
==================

Purpose
-------

`libpkgstate` owns durable installed package truth.

It represents what one managed target records as installed, owned, and required
for future package operations.  It does not reconstruct that truth from an
archive, a current package source, a live filesystem scan, or current policy.

The canonical state model contains:

```text
package release
installed package
installed ownership
installed control
state target binding
installed-state snapshot
state-publication request
state-publication receipt
```

The implementation currently exposes canonical package releases, immutable
installed control, durable target-state bindings, complete canonical installed
packages, and identified immutable snapshots.  Canonical package records,
ownership inventories, and snapshots compute their own state-domain identities.

The historical `/var/lib/pkg/db` backend uses explicitly separate
`legacy_installed_package` and `legacy_snapshot` values.  Those compatibility
values preserve validated name and opaque version lines plus canonical ownership
paths without pretending that the old format contains release, control, target,
evidence, or typed installed identities.

This document is normative for the direction of the public model and storage
interfaces.  Existing behavior remains current until a later commit implements
and documents its replacement.  Documentation must not describe an intended
interface as already available.

Authority boundary
------------------

Package management crosses several independent truth domains:

```text
source truth          libpkgsource
archive truth         libpkgimage
planning truth        libpkgplan
application truth     libpkgapply or the application boundary
installed truth       libpkgstate
filesystem truth      an explicit observation backend
orchestration policy  pkgman
```

`libpkgstate` consumes completed application outcomes and explicit publication
requests.  It returns immutable installed snapshots and publication receipts.
It does not decide installation policy or execute package operations.

```text
candidate and artifact facts
             |
             v
        libpkgplan
             |
             v
  package-operation plan
             |
             v
 application boundary
             |
             v
 completed application evidence
             |
             v
        libpkgstate
             |
             v
 installed snapshot and publication receipt
```

The library does not:

* open package archives;
* replay package payloads;
* inspect the live target filesystem;
* evaluate preserve, replace, omit, or reject policy;
* create rejected objects;
* execute lifecycle or maintenance actions;
* resolve dependencies;
* compose a complete target-system context;
* coordinate a complete package transaction; or
* claim atomicity with filesystem changes.

Archive truth and installed truth
---------------------------------

`libpkgimage` and `libpkgstate` intentionally describe different realities.

A package image states what one exact archive contains.  Installed ownership
states what durable package state records after completed installation policy.
An archive entry may be preserved, rejected, omitted, replaced, shared, or
otherwise resolved before state publication.

The following equations are forbidden:

```text
package image == installed ownership
archive filename == package identity
planned ownership == completed installed ownership
```

`libpkgstate` must not create an installed package by copying package-image
entries.  A higher application boundary supplies completed ownership outcomes
and evidence.  State validates and publishes those outcomes.

Installed truth and observed filesystem truth
---------------------------------------------

Installed truth is durable recorded state.  Observed filesystem truth is a
bounded observation of a root view at one time.

A scan may show that durable state is stale.  It does not silently rewrite
installed truth.  Audit and reconciliation return facts to a consumer; they do
not acquire package-state mutation authority merely by detecting a mismatch.

An installed snapshot therefore does not claim that the live filesystem still
matches its ownership records.  Plans and applications that require freshness
must carry and validate explicit filesystem observations separately.

Canonical package release
-------------------------

A canonical package release has four fields:

```text
package_release_identity
name
version
release
```

The identity is an authority-bearing digest.  The three descriptive fields are
validated coordinates of the same release.  `package_release::make()` computes
the identity from a domain-separated canonical record containing `name`,
`version`, and `release` in that order.  The caller cannot pair arbitrary digest
bytes with different coordinates.

Version ordering remains outside `libpkgstate`; canonical record ordering is not
package-version precedence.  A planning adapter may copy the exact
algorithm-qualified identity into the corresponding `libpkgplan` domain, where
it remains a caller-supplied authority fact rather than a planner calculation.

The historical database stores only:

```text
name
opaque version line
```

A value such as `1.0-1` cannot be decomposed reliably into upstream version and
distribution release.  Hyphens may belong to either coordinate, and the legacy
format did not record the boundary.

Consequently:

* the legacy version line remains an opaque compatibility fact;
* reading legacy state must not guess a release decomposition;
* a complete canonical package release requires explicit authoritative input;
* migration-supplied decomposition must be marked as migration provenance; and
* adapters that require complete release facts must refuse incomplete records.

`package_identity` is now quarantined as the compatibility value used only by
historical package records.  Its line-safety validation remains valid for the
legacy format, but it is not a canonical installed-package identity and its
second field remains an opaque version line.

Canonical package paths
-----------------------

Canonical installed state owns its logical package-path value and validation
contract.  A package path is root-relative, normalized, and independent of the
pathname at which a target root happens to be mounted.

`pkgstate::package_path` owns this contract.  Its normalization currently
matches `pkgimage::package_path`, but the public types are independent.  Any
cross-domain conversion remains explicit and must be checked by narrow adapters
and shared conformance vectors rather than by a public type alias or dependency.

Installed ownership
-------------------

Installed ownership records which package owns each canonical logical package
path after completed application policy.

Shared ownership is valid state.  More than one installed package may own the
same path.  State must preserve every ownership claim in deterministic order;
it must neither collapse the relation to one owner nor infer that sharing is an
error.

Canonical ownership is recorded once in the per-package manifests:

```text
installed package record -> canonical owned paths
```

The snapshot derives the aggregate path-to-owner inventory and reverse lookup
index from those manifests.  The derived inventory has its own identity for
planning and precondition binding, but it is not a second independently mutable
copy of ownership truth.

The compatibility database preserves only two object classes:

```text
directory
non_directory
```

`non_directory` may represent a regular file, symbolic link, FIFO, socket, or
device.  Legacy import must not invent more precise metadata than durable
storage contains.  A future canonical backend may retain richer recorded object
facts, but unknown metadata remains unknown rather than reconstructed from an
archive or live observation.

An empty ownership manifest is representable.  It means that the installed
package record owns no paths.  It must not be confused with an ownership set
whose historical contents are unavailable.

Installed package
-----------------

A canonical installed package is the durable fact that one package release is
installed in one installed-state snapshot.

It binds:

* canonical package release;
* installed package identity;
* installed-control identity;
* completed per-package ownership manifest;
* target-state binding; and
* application and transaction evidence supplied for publication.

Its canonical content identity covers those state-owned fields and references.
Computing that identity while constructing a publication request does not by
itself claim that the package is installed.  Installed authority begins only
when a successful publication receipt and resulting snapshot cite the record.

The containing snapshot and publication receipt bind the installed package to
the resulting snapshot identity.  The installed-package identity itself must
not contain the snapshot identity, because the snapshot contains the installed
package and that would create an identity cycle.

An installed package is not a package image, artifact manifest, source
candidate, current source directory, or live filesystem subtree.

The installed package identity is assigned by `libpkgstate` from the canonical
installed record.  Callers may supply the facts and subordinate evidence, but
they do not select an arbitrary installed package identity and ask state to
bless it.

The public `installed_package::make()` constructor implements that boundary. It
requires one `package_release`, `installed_control` for exactly that release,
one `state_target_binding`, and one completed ownership manifest. It normalizes
and validates the manifest, then computes `installed_package_identity` from the
release, control, target-binding, and normalized ownership facts. Application
and transaction evidence participate through installed-control provenance.

Installed control
-----------------

Installed control contains durable non-payload facts required after the
candidate, provider, source snapshot, or build cache has disappeared.

At minimum it must be able to retain:

* canonical package release;
* runtime dependency declarations selected at installation;
* exact pre-remove and post-remove lifecycle declarations;
* package target or profile facts needed for later reasoning;
* candidate-control provenance;
* artifact and artifact-manifest provenance;
* application and transaction evidence references; and
* explicit completeness and provenance for compatibility-sourced fields.

Lifecycle declarations must contain durable executable material or durable
content-addressed references sufficient to retrieve the exact material.  A
source-tree pathname, candidate pathname, or temporary source-snapshot root is
not durable installed control.

A candidate-control identity alone is not sufficient.  Removal must remain
possible when the candidate database and provider are unavailable.  State must
not reopen current source or candidate storage to reconstruct historical
removal actions or runtime requirements.

Known absence and historical unavailability are distinct:

```text
known empty               the authority recorded that no values exist
historically unavailable  the compatibility format did not retain the facts
```

A single Boolean completeness flag is insufficient where those states differ.
Canonical records must retain field-level or group-level completeness adequate
to distinguish them.

The public `installed_control` value implements this durable layer independently
of installed ownership.  It normalizes runtime dependency declarations as a
set, preserves ordered pre-remove and post-remove declarations as exact bytes,
retains named target/profile facts, and stores canonical external provenance
identities without recomputing their authority.

Each control group records one of four historical states:

```text
recorded at installation
recorded in compatibility storage
supplied by explicit migration
historically unavailable
```

An empty group in any of the first three states is known empty.  A historically
unavailable group must contain no values.  There is deliberately no state for
facts inferred from current mutable sources, candidates, archives, or the live
filesystem.

State target binding
--------------------

`pkgman` composes the complete target-system context.  `libpkgstate` owns and
validates only the durable state projection required to identify its storage
and publication domain.

A canonical `state_target_binding` binds at least:

```text
managed target identity
state store identity
root-view identity
state backend identity
publication-domain identity
```

A pathname such as `/` or `/altroot` is only a locator.  It is not sufficient
identity because mount namespace, bind mounts, filesystem replacement, symlink
resolution, and backend selection can change what the same string denotes.

The complete target-system context also contains the installed snapshot,
package target profile, observation and application backends, execution
capabilities, configuration scopes, and mutation-lock domain.  Those wider
operational facts remain caller-owned.

The installed snapshot must not contain the complete target-system-context
identity.  The complete context includes the installed snapshot identity, so
including the context identity in the snapshot would create a recursive digest:

```text
snapshot identity
    -> target-system-context identity
        -> snapshot identity
```

The snapshot instead contains the narrower state target binding.  `pkgman`
composes the complete target context from that binding plus the snapshot and
other operational facts.

The public `state_target_binding` value implements that narrow projection.  It
retains typed managed-target, state-store, root-view, state-backend, and
publication-domain identities supplied by their owning authorities, then
computes a state-owned binding identity from their canonical record.  It
contains neither a root pathname nor an installed-snapshot identity.

Canonical identities
--------------------

Every durable semantic object has a strongly typed identity domain.  At
minimum, the canonical model requires:

```text
package_release_identity
installed_control_identity
installed_package_identity
state_target_binding_identity
ownership_inventory_identity
installed_state_snapshot_identity
state_publication_request_identity
state_publication_receipt_identity
```

The identity dependency graph is acyclic:

```text
release + control + manifest + target binding + evidence
                         |
                         v
                installed package
                         |
                         v
                ownership inventory
                         |
                         v
             installed-state snapshot
```

Publication identities extend that graph:

```text
expected snapshot + transition + completed evidence
                         |
                         v
              publication request
                         |
                         v
              publication receipt
```

Canonical digest text uses an algorithm-qualified representation:

```text
v1:sha256:<lowercase hexadecimal>
```

Each semantic identity remains a distinct C++ type even when two values use the
same representation and algorithm.  A package-release identity cannot be
passed where a snapshot identity is required merely because both contain 32
SHA-256 bytes.

Canonical record encoding
-------------------------

Identity is computed over versioned, domain-separated canonical records.
Domain labels are state-owned and include a format version, for example:

```text
pkgstate/package-release/1
pkgstate/installed-control/1
pkgstate/installed-package/1
pkgstate/target-binding/1
pkgstate/ownership-inventory/1
pkgstate/installed-snapshot/1
pkgstate/publication-request/1
pkgstate/publication-receipt/1
```

Canonical records use one common envelope:

```text
9 bytes   literal `pkgstate` followed by NUL
u16be     canonical-record encoding version, currently 1
u64be     domain-label byte length
octets    exact domain-label bytes
...       schema-owned fields
```

Schema fields use unsigned big-endian integers, one-byte Boolean or optional
markers with values 0 and 1, and byte strings prefixed by an unsigned 64-bit
big-endian length.  An embedded digest is encoded as its unsigned 16-bit
representation version, unsigned 16-bit algorithm identifier, unsigned 64-bit
byte length, and exact digest bytes.  Lists and maps must encode an explicit
count and define canonical member ordering before hashing.

Canonical identity must never depend on:

* C++ object layout or padding;
* pointer values;
* enum ABI representation without an explicit canonical mapping;
* insertion order of an unordered container;
* diagnostic text;
* storage pathnames;
* inode numbers or process-local descriptors; or
* presentation-only whitespace.

The reverse ownership index is derived and excluded as a second serialized
copy.  Canonical vectors must prove that arbitrary input order yields identical
normalized records and identities.

Installed-state snapshot
------------------------

A canonical installed-state snapshot is one complete immutable fact universe
for installed-state planning and publication.

It binds:

* snapshot schema version;
* state target binding;
* canonical schema and completeness profile;
* normalized installed package records;
* installed control records;
* canonical ownership relation;
* ownership-inventory identity; and
* installed-state snapshot identity.

Construction must validate:

* deterministic package and path ordering;
* unique installed package names within the declared namespace;
* one installed control for every installed package;
* no orphan installed controls;
* no duplicate ownership claim;
* no ownership claim naming an absent package;
* consistency between installed package and control release bindings; and
* explicit completeness for every compatibility limitation.

The public `snapshot::make()` constructor implements the complete identified
fact container. It accepts one state target binding and complete canonical
installed packages only, rejects packages from another binding and duplicate
package names, and derives exact shared ownership from the per-package
manifests.

The ownership-inventory identity is computed over path-ordered ownership groups.
Each group contains one canonical path and its installed-package identities in
identity order. Owner package names are not substituted for installed-package
identities, so a changed canonical owner record changes the admitted ownership
relation even when its name and paths remain the same.

The installed-state snapshot identity is computed over the explicit schema
version, state-target-binding identity, installed-package identities in package
name order, and ownership-inventory identity. The reverse ownership index is
not hashed as a second implementation-dependent copy.

A canonical snapshot remains immutable after construction. It does not refresh
itself after another process publishes state. A consumer requiring current truth
must read a new snapshot.

State-publication request
-------------------------

A state-publication request is an immutable request to publish completed
installed-state consequences against one expected prior snapshot.

The primitive transition is a one-package delta derived from one accepted
package-operation plan and completed application evidence.  A transaction-level
request may compose more than one non-conflicting package delta so one state
publication can record a completed multi-package transaction.  A request is not
an arbitrary complete snapshot authored by the caller, and it cannot silently
reassert unrelated installed truth.

A request binds:

```text
expected prior snapshot identity
state target binding
one or more package-operation deltas
operation-plan identity for every delta
completed application evidence identity for every delta
affected old installed package, where applicable
proposed installed package facts and control, where applicable
completed ownership transition
explicit package absence, for removal
transaction evidence, when deltas are composed
publication request identity
```

The public `package_state_delta` factories implement the one-package
transition. Install requires prior absence and one complete proposed installed
package. Replace requires one exact old installed-package identity and one
different complete proposed package with the same name. Remove requires one
exact old installed-package identity and explicit package absence.

Every delta contains one caller-authoritative operation-plan identity and one
completed application-evidence identity. Install and replacement require the
proposed installed control to retain matching application evidence. The public
`state_publication_request::make()` constructor accepts one complete expected
snapshot, validates every delta against it, sorts deltas by package name, and
rejects duplicate package names. More than one delta requires transaction
evidence; proposed packages retain matching transaction provenance whenever
transaction evidence is present.

The request identity covers the request schema version, expected snapshot,
target binding, normalized deltas, old and proposed installed-package
identities, accepted plans, completed application evidence, and optional
transaction
evidence. External evidence values use typed canonical digest references but
remain caller-authoritative; libpkgstate does not claim to have computed or
authenticated them.

A request contains installed-state consequences and evidence.  Proposed
installed records are not yet installed truth; state validates their canonical
identities and the receipt records whether they entered the resulting snapshot.
The request contains no filesystem mutation instructions, payload replay source,
preserve policy, lifecycle execution policy, or maintenance action. It is not a
caller-authored complete replacement snapshot.

Planning publication intent is not sufficient evidence.  A failed or partial
application cannot be published as successful installed truth merely because a
plan described the desired result.  The application boundary must report the
completed effects actually eligible for publication.

Compare-and-publish
-------------------

Canonical publication is compare-and-publish under the backend's publication
lock.

The backend must:

1. acquire its exclusive state-publication lock;
2. reread durable state under that lock;
3. compute or validate the actual snapshot identity;
4. compare it with the request's expected snapshot identity;
5. refuse without mutation when the identities differ;
6. apply every validated, non-conflicting delta to the actual snapshot;
7. compute and validate the resulting snapshot;
8. publish according to the backend's declared atomicity boundary; and
9. return a structured publication receipt.

An exclusive lock does not replace the comparison.  A plan may have been
created from snapshot A, another writer may publish snapshot B, and the first
writer may acquire the lock only afterwards.  Beginning a write from B and
blindly applying an A-based result would accept stale planning authority.

A stale-state refusal is an ordinary typed publication outcome.  It is not an
I/O exception and must not mutate durable state.

State-publication receipt
-------------------------

A publication receipt is immutable evidence of one publication attempt.

It records at least:

```text
receipt identity
request identity
expected prior snapshot identity
actual prior snapshot identity
target and store binding
resulting snapshot identity, when established
backend identity and storage format
publication outcome
durability outcome
actual state-storage atomicity boundary
subordinate evidence identities
```

The outcome taxonomy must distinguish at least:

```text
published
stale expected state
request rejected before publication
failed before publication
published but durability confirmation failed
publication outcome indeterminate; reread required
```

A Boolean `committed()` value cannot express these states.  In particular, a
backend may rename a new state into place and then fail to synchronize the
containing directory.  The new state may be visible even though durable
confirmation failed.  The receipt must record that boundary without claiming
that the old state remained authoritative.

A receipt records state-storage publication only.  It must not imply that
filesystem application, rejected-object staging, lifecycle execution,
maintenance, and installed-state publication were globally atomic.

Canonical storage backend
-------------------------

The complete canonical model requires a backend that can store rich installed
control, typed identities, target binding, completeness, and publication
receipts without loss.

The preferred publication shape is immutable generations plus an atomically
selected current generation:

```text
construct complete generation
write and synchronize generation
atomically select generation as current
synchronize selection domain
finalize a receipt from the actual publication outcome
```

A generation contains one complete installed snapshot and the durable records
required to validate it.  Incomplete prepublication generations are never
current and may be collected safely according to explicit garbage-collection
policy.

The exact on-disk encoding and generation layout are separate storage-design
work.  They must preserve the semantic contracts in this document and declare
their actual crash and durability boundaries.

Legacy compatibility backend
----------------------------

`legacy_text_store` implements the historical `/var/lib/pkg/db` format with
original Zeppe-Lin code.

The legacy format carries:

```text
package name
opaque version line
directory or non-directory ownership paths
```

It does not carry complete canonical package releases, installed control,
target binding, typed identities, lifecycle material, runtime requirements,
application evidence, or publication receipts.

The public compatibility projection is explicit:

```text
package_identity
legacy_installed_package
legacy_snapshot
store / write_transaction
legacy_text_store
```

Canonical `installed_package` and `snapshot` values are different types and
cannot be passed to the compatibility transaction interface accidentally.

The compatibility backend therefore has four obligations:

* read existing valid databases without automatic rewrite;
* expose unavailable canonical facts explicitly as unavailable;
* preserve byte-compatible legacy serialization for representable records; and
* reject attempts to down-convert complete canonical records when doing so
  would discard required facts.

A compatibility read is an adapter from a limited durable format.  It must not
make the old format appear complete by deriving missing facts from current
source trees, archives, candidate databases, package filenames, or
configuration.

The legacy database must not be extended casually with independent sidecar
files.  Two or more files do not form one atomic state publication merely
because they share a directory.  A sidecar design would require an explicit
multi-object generation and selection protocol, at which point it is a new
backend rather than an invisible extension of the old format.

The existing `write_transaction` remains a compatibility transaction while the
canonical publication interface is introduced.  It protects legacy storage
mutation but does not provide caller-supplied expected-state comparison and
must not be described as the canonical stale-safe publication API.

Migration
---------

Migration from legacy state is explicit and receipt-bound.

A migration operation must:

* identify the source backend and exact source observation;
* identify the destination target and canonical store;
* preserve every fact the legacy format actually carries;
* mark unavailable release, control, and provenance facts as unavailable;
* accept supplementary facts only from explicit named migration inputs;
* record which fields came from legacy storage and which were supplied;
* perform no source-tree or provider reconstruction behind the caller's back;
* support a non-mutating validation or dry-run path; and
* return a structured migration receipt.

Reading legacy state is not migration.  A read must never rewrite or enrich the
source database automatically.

Adapters and dependency direction
---------------------------------

`libpkgstate` must not depend on `libpkgplan` or `libpkgapply`.

The planner consumes caller-authoritative installed-package, installed-control,
ownership-inventory, installed-snapshot, and target-context identities.  State
owns the corresponding durable facts and their identities.  Their semantic
representations may be wire-compatible without sharing planner-owned C++ types.

The dependency direction is:

```text
libpkgstate      libpkgplan
      \            /
       \          /
        adapter or pkgman
```

An optional adapter may depend on both libraries.  It may copy the
algorithm-qualified bytes from one strong identity domain into the matching
planner domain after validating representation and completeness.  It must not
convert incomplete legacy facts into complete planner facts.

The adapter also must not derive a complete target-system-context identity from
a state snapshot.  The caller supplies the composed target context; the adapter
verifies that its durable state projection agrees with the snapshot's target
binding.

A future application adapter may translate completed application evidence into
a state-publication request.  It does not grant `libpkgstate` authority over
application semantics, and it does not grant the application layer authority
to assign canonical installed identities.

Reference frontend
------------------

`pkginfo(1)` is a small composition client for the compatibility state backend
and the `libpkgimage` archive backend.

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

Canonical state diagnostics should use a separate non-mutating frontend or
explicit new modes.  Existing `pkginfo -i`, `-l`, and `-o` output must not gain
identity or completeness fields accidentally during migration.

Determinism
-----------

Determinism is part of the public contract.

The following are deterministic:

* package-release canonical records;
* installed-control canonical records;
* installed package ordering;
* ownership claim ordering;
* shared-owner ordering;
* installed snapshot identity;
* publication request and receipt identity;
* compatibility database serialization; and
* existing `pkginfo(1)` line order.

Canonical state constructors accept values in arbitrary caller order where the
semantic object is unordered, normalize them, reject duplicates or
contradictions, and expose the canonical order.

Concurrency and atomicity
-------------------------

Snapshots and receipts are immutable after construction and may be read
concurrently when their contained values provide ordinary immutable-value
semantics.

Storage concurrency is backend-specific.  Every backend must document:

* lock scope and interoperability;
* whether reads block, wait, or fail on contention;
* compare-and-publish serialization;
* publication and durability boundaries;
* post-failure reread requirements; and
* recovery authority.

A package-database lock alone is not a target mutation lease.  The complete
transaction coordinator must protect the package-owned filesystem and state
publication under one declared target mutation domain.  `libpkgstate` validates
the state-publication projection of that authority but does not own the complete
lease protocol.

Process-group handling, signal deferral, temporary files, rename, and directory
synchronization are mechanisms.  None by itself proves global package
transaction atomicity.

Errors and typed outcomes
-------------------------

Programming errors, invalid records, storage failures, and ordinary publication
outcomes are different classes.

Exceptions remain appropriate for:

* malformed identity representations;
* invalid state object construction;
* storage I/O or locking failures that prevent a structured result;
* backend corruption; and
* invalid transaction lifecycle use.

Structured result objects are required for expected semantic outcomes such as:

* stale expected state;
* incomplete compatibility facts;
* unsupported down-conversion;
* publication rejection; and
* publication whose visibility or durability requires reread.

Consumers must select behavior from typed error or outcome classes.  Diagnostic
text is presentation and must not become a machine-readable protocol.

Forbidden shortcuts
-------------------

The following designs violate the authority model:

1. Parsing a legacy version line into version and release by convention.
2. Treating current candidate control as durable installed control.
3. Retaining temporary source-snapshot paths in installed state.
4. Copying a package image into installed ownership.
5. Embedding complete target-context identity in snapshot identity.
6. Hashing a database pathname to invent target or store identity.
7. Calling exclusive locking alone stale-state protection.
8. Publishing an A-based result after silently rebasing it onto snapshot B.
9. Extending the legacy database with uncoordinated sidecars while claiming one
   atomic publication.
10. Letting an application layer assign authoritative installed identities.
11. Making the state core depend on planner or application C++ types.
12. Publishing intended plan outcomes after partial or failed application.
13. Treating a receipt as proof of filesystem/state global atomicity.
14. Reconstructing historical runtime or lifecycle facts from current source.

Implementation gates
--------------------

The canonical implementation may proceed only in dependency order:

1. freeze identity domains and canonical records;
2. add structured package release and installed control;
3. add state target binding and complete immutable snapshots;
4. add publication requests and typed receipts;
5. implement compare-and-publish;
6. add a lossless canonical backend;
7. adapt legacy state as explicitly incomplete;
8. add explicit migration; and
9. add optional planner and application adapters.

A compatibility frontend may preserve old commands and storage formats during
this transition.  Compatibility skin must not become the canonical ontology.
