# History

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
