# Migration boundary

This document records the explicit migration boundary and historical release transitions. Version-specific sections describe the named releases; they are not claims about the current 3.0 repository surface.

`libpkgstate` 2.0.0 does not read or write the historical CRUX package database.
It does not contain a compatibility model or a legacy import API.

Version 2.0.0 also does not perform an in-place upgrade of generation-v1 or
generation-v2 canonical storage. Do not point a 2.0.0 writer at an existing
0.6.0 or 1.0.0 state store and expect reinterpretation or repair. Native
publication begins with a fresh state target.

A future standalone importer must be treated as an observation and admission
program, not as a backend mode. It must:

1. read the historical database or old generation store without modifying it;
2. identify exactly which facts are retained and which are absent;
3. require explicit package release, source, build, lifecycle, architecture,
   installation-reason, target, application, and ownership evidence for absent
   facts;
4. construct complete native installation receipts or a separately reviewed
   bootstrap contract;
5. publish only into a fresh native state target; and
6. emit durable migration evidence.

A generation-v2 importer cannot treat its planner candidate-control and artifact
manifest references as native build authority. It must obtain or explicitly
bootstrap the request, source-material, materialized-input, policy, result,
payload, artifact-content, execution, image, and inspection facts required by
the 2.0.0 model.

The importer must never split opaque old version lines by guesswork, substitute
current recipes for historical source authority, infer package meaning from
archive filenames, or counterfeit absent build/application evidence.
## 2.0.0 to 2.1.0 application API

Generation-v3 storage and core installed-state identities are unchanged. No
state-store migration is required.

The `libpkgstate-apply` ABI is incompatible. Consumers must stop constructing
`incoming_installation_authority` and must pass the exact typed libpkgapply
installation, upgrade, or removal request named by completed evidence.
Installation additionally supplies only its typed installation reason. Upgrade
preserves the reason already stored for the replaced package.

There is no compatibility overload accepting a separately projected build
authority. Rebuild consumers against `libpkgstate-apply` soversion 3 and
`libpkgapply` 1.0.0.

## 2.1.0 to 2.2.0 transaction provenance

Generation-v3 storage and every SONAME remain unchanged. No state-store
migration or consumer rebuild is required.

Existing `project_completed_application()` overloads continue to produce
publication requests and installation receipts without transaction evidence.
Transaction controllers should call the additive overload matching the exact
installation, upgrade, or removal request and supply one
`transaction_evidence_identity`. The adapter retains that evidence in the
publication request and, for installation or upgrade, in the proposed package's
durable installation receipt.

Do not project first and attempt to add transaction evidence afterward. The
state model requires the proposed package receipt and publication request to
carry the same transaction evidence at construction time.

## 2.2.0 to 2.3.0 authority closure

No state-store migration is required. Rebuild and reinstall all adapters after
installing `libpkgsource 2.0.0`, `libpkgbuild 2.0.0`, and `libpkgapply 2.0.0`.
The adapter SONAMEs remain stable because they project external authority by
reference into unchanged native state values; their runtime dependency closure
must nevertheless be generation-consistent.

## 2.3.0 to 2.4.0 lease-bound application state

No durable state migration is required. Canonical generation storage remains
`libpkgstate-generation-v3`, core soversion remains 3, and the application
adapter soversion remains 3. Consumers that assemble native application effects
should replace caller-constructed `lease_bound_state_projection` values with
`read_application_state()`, passing the exact application request, live
libpkgapply 2.1 target mutation lease, and canonical store. The returned snapshot
and projection must remain paired for the lifetime of the effect driver.


## 2.4.0 to 2.5.0 durable publication evidence

No state-store migration is required. Canonical generation storage remains
`libpkgstate-generation-v3`, core soversion remains 3, and every adapter
soversion remains unchanged.

Controllers that persist state-publication checkpoints may encode the immutable
request and receipt. Request decode must be supplied the exact expected snapshot.
Receipt decode must be supplied that reconstructed request and the exact actual
prior snapshot observed by the publication attempt. Do not reconstruct those
semantic bodies from their digest strings or substitute the current store state
for the retained actual prior.


## 2.5.0 to 2.5.1 publication framing correction

No installed-state store migration is required. Generation storage remains
`libpkgstate-generation-v3` and every SONAME remains unchanged.

New request and receipt encodings use fixed eight-byte house framing and schema
version 2:

```text
ZLSPRQST  state-publication request
ZLSPRCPT  state-publication receipt
```

The 2.5.1 decoders continue to admit canonical version-1 records emitted by
2.5.0, but all public encoders emit version 2. There is no dual-write option,
store scanner, or migration utility because these evidence records have not yet
entered production storage. Controllers may simply retain any existing 2.5.0
bytes and allow a later semantic rewrite to encode version 2.


## 2.5.1 to 3.0.0 repository extraction

No installed-state migration is required. Core SONAME advances from 3 to 4 because the concrete generation-store class moves to `libpkgstate-posix`; publication evidence schema 2 and generation-v3 bytes remain unchanged.

The source, build, planner, and application adapters and the concrete generation provider are no longer built by `libpkgstate`. Install the independently released `libpkgstate-source`, `libpkgstate-build`, `libpkgstate-plan`, and `libpkgstate-apply` repositories at matching 3.0 generation and `libpkgstate-posix` for generation-v3 storage, then relink consumers against the exact adapter product they use. Remove the former `source_adapter`, `build_adapter`, `planner_adapter`, and `application_adapter` Meson options from packaging and integration scripts.

The extraction does not add a compatibility library, umbrella adapter package, migration mode, or transitive core dependency.
