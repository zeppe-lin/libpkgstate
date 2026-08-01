# History

## 2.5.0

Durable state-publication evidence:

- add canonical request and receipt encodings to the core library;
- require the exact expected snapshot to decode a publication request;
- retain complete proposed installed-package bodies without serializing the
  expected snapshot as caller-replaceable authority;
- require the exact request and actual-prior snapshot to decode a receipt;
- derive successful or indeterminate resulting state from those exact bodies;
- distinguish malformed records from expected-snapshot, request, actual-prior,
  and recomputed-identity mismatches; and
- add whole-record SHA-256 checksums, refusal bounds, deterministic re-encoding,
  and direct corruption/substitution coverage.

ABI and storage decisions:

- project version becomes 2.5.0;
- core `libpkgstate` remains at soversion 3;
- every adapter soversion remains unchanged;
- generation-v3 installed-state storage does not change; and
- no publication codec opens a store, invokes a backend, or publishes state.

## 2.4.0

Lease-bound application-state projection release.

- add `read_application_state()` to `libpkgstate-apply`;
- require one explicit caller-owned libpkgapply 2.1 target mutation lease and
  one canonical store;
- perform exactly one store read while the lease is live and recheck lease
  retention after the read and before returning;
- return the exact canonical snapshot together with its complete accepted-plan
  path-owner projection as one value;
- derive state-projection evidence canonically from the request, target, lease
  acquisition, state target, snapshot, ownership inventory, and path closure;
- add no target observation, application execution, publication, reconciliation,
  repair, store initialization, lease acquisition, waiting, or retry policy;
- raise the `libpkgstate-apply` dependency floor to libpkgapply 2.1.0 while
  retaining adapter soversion 3 and core soversion 3; and
- retain `libpkgstate-generation-v3` without migration.

Generation-v3 storage does not change in this release. No durable state
migration is required between 2.3.0 and 2.4.0.

## 2.3.0

Generation-2 authority-closure migration release.

- Rebuilt `libpkgstate-source` against `libpkgsource 2.0.0`.
- Rebuilt `libpkgstate-build` against `libpkgbuild 2.0.0`.
- Rebuilt `libpkgstate-apply` against `libpkgapply 2.0.0`.
- Raised all adapter pkg-config floors so generation-1 source/build/application
  libraries cannot re-enter a native process.
- Core `libpkgstate` remains at soversion 3; source, build, planner, and
  application adapter SONAMEs remain unchanged.
- Generation-v3 storage does not change in this release. No durable state
  migration is required.

## 2.2.0

Transaction-provenance projection release. Version 2.2.0 keeps the
generation-v3 installed-state representation and every existing ABI while
allowing an effectful transaction controller to retain one exact transaction
evidence identity during completed application projection.

Authority transition:

- add installation, upgrade, and removal projection overloads that accept one
  exact `transaction_evidence_identity`;
- retain that identity in the resulting `state_publication_request`;
- retain the same identity in the durable `installation_receipt` produced for
  installation or upgrade;
- preserve the existing no-transaction-evidence overloads and their identities;
  and
- reject no publication model invariant: the adapter supplies the same evidence
  to both the proposed package receipt and publication request rather than
  trying to attach transaction provenance after projection.

ABI and representation decisions:

- project version becomes 2.2.0;
- core `libpkgstate` remains at soversion 3;
- source, build, planner, and application adapter SONAMEs remain unchanged; and
- canonical storage remains `libpkgstate-generation-v3`.

Generation-v3 storage does not change in this release. No state-store migration
or consumer rebuild is required between 2.1.0 and 2.2.0.

## 2.1.0

Native application-admission release. Version 2.1.0 keeps the generation-v3
installed-state representation and core ABI while making the destination-owned
application adapter consume request-bound native build authority.

Authority transition:

- require `libpkgapply 1.0.0` installation and upgrade requests, whose admitted
  incoming package retains the complete successful build result, independent
  image inspection, and source-derived planner candidate control;
- remove the caller-supplied `incoming_installation_authority` wrapper from the
  public state application adapter;
- derive `package_source_record` and `build_authority` only from the exact
  incoming package retained by the completed application request;
- accept only an initial typed installation reason for installation, preserve
  the prior installed reason for upgrade, and accept no incoming authority for
  removal; and
- verify that accepted plan publication and completed application evidence bind
  the same artifact and request universe before constructing installed state.

ABI and representation decisions:

- project version becomes 2.1.0;
- core `libpkgstate` remains at soversion 3;
- source, build, and planner adapter SONAMEs remain unchanged;
- `libpkgstate-apply` advances to soversion 3; and
- canonical storage remains `libpkgstate-generation-v3`.

Generation-v3 storage does not change in this release. No state-store migration
is required between 2.0.0 and 2.1.0, but application-adapter consumers must be
rebuilt for the new request-bound API.

## 2.0.0

Native build-authority release. Version 2.0.0 is intentionally incompatible
with the 1.0.0 build-provenance ABI and generation storage.

Authority and model:

- replace planner-derived candidate-control and artifact-manifest placeholders
  with complete source-bound native build provenance;
- retain the sealed build request, verified source materials, materialized build
  inputs, environment policy, build policy, successful build result, payload
  manifest, sealed artifact, exact artifact content, artifact binding,
  execution evidence, normalized image, and image-inspection identities;
- require every installed control to bind build provenance to the same exact
  package source record; and
- advance state-owned identity domains affected by the new authority record.

Composition boundaries:

- add `libpkgstate-build` to admit only complete successful libpkgbuild 1.0.0
  results whose artifact bytes and normalized payload exactly match an
  independent libpkgimage 0.3.0 inspection;
- make `libpkgstate-apply` consume that admitted build authority instead of
  caller-supplied build identity fragments;
- verify that the accepted planner operation names the exact admitted artifact
  content; and
- keep source, build, image, planner, and application dependencies outside the
  core shared-library closure.

ABI and representation decisions:

- project version becomes 2.0.0;
- core `libpkgstate` advances to soversion 3;
- `libpkgstate-source` remains at soversion 1;
- `libpkgstate-build` begins at soversion 1;
- `libpkgstate-plan` remains at soversion 2;
- `libpkgstate-apply` advances to soversion 2; and
- canonical generation storage becomes `libpkgstate-generation-v3`.

This release contains no in-place migration path. Generation-v1 and
generation-v2 stores require a separate explicit migration program that admits
complete native source, build, application, and ownership authority into a fresh
generation-v3 target.

## 1.0.0

First native installed-state authority release. Version 1.0.0 is intentionally
incompatible with the CRUX-shaped 0.6.0 model, ABI, and generation storage.

Authority and model:

- remove the historical CRUX database parser, compatibility observations,
  mutable transaction API, legacy import API, and old-format store backend from
  the authoritative library;
- retain exact libpkgsource 1.0.0 package-release, selected-profile, recipe, and
  source-snapshot identities instead of recreating source authority from package
  coordinates;
- require complete package source records, typed installation reasons, build,
  artifact, plan, application, target, and ownership evidence;
- retain typed runtime requirements, lifecycle programs, action-bound lifecycle
  requirements, and build/target architecture selections;
- require complete installation receipts before an installed package can exist;
  and
- reject incomplete native installed records rather than encoding historical
  absence as current authority.

Publication and storage:

- preserve stale-safe compare-and-publish, with the store deriving the only
  admissible resulting snapshot from caller deltas; and
- advance canonical storage to `libpkgstate-generation-v2` without interpreting
  version 1 bytes as native version 2.

Composition boundaries:

- add `libpkgstate-source` for exact projection from libpkgsource 1.0.0;
- reset `libpkgstate-plan` for libpkgplan 0.2.0; and
- add `libpkgstate-apply` for completed libpkgapply 0.1.0 evidence.

ABI and representation decisions:

- project version becomes 1.0.0;
- core `libpkgstate` advances to soversion 2;
- `libpkgstate-source` begins at soversion 1;
- `libpkgstate-plan` advances to soversion 2;
- `libpkgstate-apply` advances to soversion 1; and
- canonical generation storage becomes `libpkgstate-generation-v2`.

This release contains no in-place migration path. Historical database and
version-1 generation import belong to a separate explicit tool.

## 0.6.0

Last release of the pre-native model. It is not ABI or storage compatible with
1.0.0 or 2.0.0.
