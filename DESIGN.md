# Native installed-state design

## Purpose

`libpkgstate` answers one question: what package state is durably installed for
this exact managed target?

It does not answer what recipe text says, what should be selected, how to build
or apply a package, or how an obsolete database should be interpreted.

## Authority boundaries

Source authority supplies a sealed source snapshot. The state source adapter
retains exact source-owned release, profile, recipe, and snapshot identities.
The state core never recreates those identities from coordinates.

Build authority supplies one sealed request, one successful result, exact
artifact bytes, execution evidence, and an ordered payload manifest. Image
authority independently inspects those bytes. The state build adapter admits the
pair only when source projection, artifact content, inspection receipt, and every
normalized payload field agree exactly.

Application authority supplies completed target effects. The state application
adapter accepts only the unforgeable build authority produced by the build
adapter, validates the accepted plan and exact artifact-content identity, and
creates a complete installation receipt. Application admission never
reconstructs build provenance from planner facts.

State authority begins at durable admission. It owns installed control, receipt,
package, ownership, snapshot, publication request, and publication receipt
identities.

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

## Storage

The generation backend writes complete immutable state objects and atomically
selects one. Storage version 3 is not a reinterpretation of version 1 or version
2. A separate migration utility is required for any older storage.
