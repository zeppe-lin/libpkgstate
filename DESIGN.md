# Native installed-state design

## Purpose

`libpkgstate` answers one question: what package state is durably installed for
this exact managed target?

It does not answer what recipe text says, what should be selected, how to build
or apply a package, or how an obsolete database should be interpreted.

## Authority boundaries

Source authority supplies a sealed source snapshot. The independently released
`libpkgstate-source` adapter retains exact source-owned release, selected-profile,
and complete snapshot identities. The state core never recreates those identities
from coordinates.

Build authority supplies one sealed request, one successful result, exact
artifact bytes, execution evidence, and an ordered payload manifest. Image
authority independently inspects those bytes. The independently released `libpkgstate-build` adapter admits the pair only when source projection, artifact content, inspection receipt, and every
normalized payload field agree exactly.

Application authority supplies one operation-specific `libpkgapply` request,
one caller-owned live target mutation lease, and completed target effects. Before
mutation, `libpkgstate-apply` performs exactly one canonical store read
under that lease, validates the request's expected snapshot and ownership
inventory, and derives the complete accepted-plan path-owner projection. The
returned snapshot and projection are one value, and projection evidence is
computed from the exact authority universe rather than supplied by the caller.

After application, application authority supplies the completed target effects. Installation and upgrade requests retain the
complete successful build result and independent image inspection admitted by
libpkgapply. `libpkgstate-apply` projects source and build authority
from that exact request, validates the accepted plan and artifact-content
identity, and creates a complete installation receipt. When an effectful
transaction controller supplies exact transaction evidence, the adapter retains
that same evidence in both the publication request and the installation receipt.
It accepts no second caller-supplied build authority. Application admission
never reconstructs build provenance from planner facts.

State authority begins at durable admission. It owns installed control, receipt,
package, ownership, snapshot, publication request, and publication receipt
identities.


## Application admission

The independently released destination-owned `libpkgstate-apply` adapter has three typed entry points. Installation takes
an installation request, completed evidence, and one initial installation
reason. Upgrade takes an upgrade request and completed evidence, retaining the
reason already present in canonical state. Removal takes a removal request and
completed evidence and has no incoming package.

For install and upgrade, the adapter reprojects the build request's sealed
source through `libpkgstate-source`, then admits the exact retained build result
and image through `libpkgstate-build`. The resulting source record, build
provenance, planner publication, application request, completed evidence, and
expected state must all name one authority universe. A caller cannot substitute
a valid build from another request or artifact after filesystem application.

Transaction provenance is construction-time authority. The adapter overloads
that accept `transaction_evidence_identity` pass one exact value into the
publication request and into every proposed installation receipt. Projection
without transaction evidence remains available and preserves its existing
identity semantics. Evidence cannot be attached after projection because that
would permit the publication request and durable package receipt to name
different transactions.

## Canonical generation protocol

`libpkgstate` owns the canonical generation-v4 binding and complete-snapshot
record codec because those bytes encode state authority, not filesystem policy.
Decode verifies framing, version, normalized structure, every state-owned
identity, target binding, and canonical re-encoding. The codec performs no I/O
and chooses no path, lock, selector, retry, recovery, or durability mechanism.

A storage provider may persist those exact records. It must not fork or
reinterpret their encoding. `libpkgstate-posix` supplies the reference
immutable-generation filesystem mechanism and depends inward on this protocol.

## Complete native records

There is no incomplete native installed package. A package must have:

1. a complete source record;
2. a typed installation reason;
3. complete source-bound build provenance;
4. one target binding;
5. a completed ownership manifest;
6. accepted operation-plan evidence; and
7. completed application evidence.

Known empty collections remain explicit empty collections. Missing historical
facts are a migration problem and cannot be represented as native authority.

## Build provenance

Build provenance retains typed identities for the source record, build request,
verified source-material set, materialized build-input set, environment policy,
build policy, build result, payload manifest, sealed artifact, exact artifact
content, artifact binding, execution evidence, normalized artifact image, and
inspection receipt.

Planner candidate and artifact-manifest identities are not build authority and
are not stored as substitutes. Package archive filenames and mutable build
paths are labels or locations, never provenance.

## Ownership

Ownership identity is derived from path-ordered ownership groups. Each package
manifest retains complete recorded active-object metadata admitted from completed
application evidence: object kind, mode, owner, timestamp, kind-specific
content, optional hard-link topology, active origin, and rejected-object
provenance. State does not rediscover these facts from the target filesystem.

## Publication

The caller submits deltas, not a complete replacement snapshot. The store reads
actual state under its publication boundary, compares the expected snapshot, and
derives the only valid result. Stale requests return a typed stale receipt before
the backend publication primitive runs.

Publication evidence has a separate durable representation. Request encoding
retains exact delta bodies, including complete proposed installed packages, but
request decode still requires the exact expected snapshot. Receipt decode
requires the exact request and actual prior snapshot. Successful and
publication-indeterminate resulting snapshots are projected from those
authorities. A record cannot supply arbitrary replacement state or rehydrate a
snapshot from an identity alone.

The codecs are pure evidence admission. They do not open the canonical store,
acquire a publication transaction, call a backend, retry an attempt, reconcile
state, or select policy.

### Publication record framing

New durable evidence records use the house-level fixed eight-byte framing
convention: `ZL`, a two-byte authority boundary, and a four-byte record kind.
State-publication requests use `ZLSPRQST`; receipts use `ZLSPRCPT`. A big-endian
16-bit schema version follows the magic. Human-readable format names belong in
diagnostics and documentation rather than variable-length magic strings.

Current encoders emit only schema version 2. Decode retains narrow compatibility
with the published 2.5.0 textual version-1 framing and checks canonical bytes
against that original version. This compatibility does not introduce a store
migrator, dual-write policy, or another semantic authority.

## Storage-provider boundary

The core defines `canonical_store`, its lock-scoped publication transaction,
and the non-virtual stale-safe compare-and-publish sequence. It does not select
a filesystem layout, open a lock, encode a provider generation, replace a
selector, synchronize directories, or inspect a concrete store.

`libpkgstate-posix` supplies the reference immutable-generation-v4 provider.
Other providers must preserve the same semantic store contract without becoming
second owners of publication policy. Storage migration remains an explicit
provider-side admission problem.
