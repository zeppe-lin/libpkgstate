# History

## 1.0.0

First native installed-state authority release. Version 1.0.0 is
intentionally incompatible with the CRUX-shaped 0.6.0 model, ABI, and
generation storage.

Authority and model:

- remove the historical CRUX database parser, compatibility
  observations, mutable transaction API, legacy import API, and
  old-format store backend from the authoritative library;
- retain exact libpkgsource 1.0.0 package-release, selected-profile,
  recipe, and source-snapshot identities instead of recreating source
  authority from package coordinates;
- require complete package source records, typed installation reasons,
  resolved build inputs, build-result and artifact identities, and
  artifact-manifest provenance;
- retain typed runtime requirements, lifecycle programs, action-bound
  lifecycle requirements, and build/target architecture selections;
- require complete installation receipts before an installed package can
  exist;
- retain completed object ownership, metadata, content, link, device,
  hard-link, preserved-object, and rejected-object evidence; and
- reject incomplete native installed records rather than encoding
  historical absence as current authority.

Publication and storage:

- bind source, build, plan, target, completed application, and ownership
  evidence into native installation receipts;
- preserve stale-safe compare-and-publish, with the store deriving the
  only admissible resulting snapshot from caller deltas;
- advance snapshot, publication request, publication receipt, ownership,
  and immutable-generation encodings; and
- advance canonical storage to `libpkgstate-generation-v2` without
  interpreting version 1 bytes as native version 2.

Composition boundaries:

- add `libpkgstate-source` for exact projection from libpkgsource 1.0.0;
- reset `libpkgstate-plan` to project complete native installed control
  into libpkgplan 0.2.0 without flattening state-owned provenance;
- reset `libpkgstate-apply` to require explicit incoming source/build
  authority, an accepted plan, and completed libpkgapply 0.1.0 evidence;
  and
- keep source, planner, image, and application dependencies outside the
  core shared-library closure.

Removed compatibility surface:

- remove `legacy_installed_package`, `legacy_snapshot`, `legacy_import`,
  `legacy_text_store`, mutable `store`/`write_transaction`, and
  `canonical_store::import_legacy()`;
- remove the compatibility `pkginfo` frontend; and
- retain only the native read-only `pkgstate-check` diagnostic frontend.

ABI and representation decisions:

- project version becomes 1.0.0;
- core `libpkgstate` advances to soversion 2;
- `libpkgstate-source` begins at soversion 1;
- `libpkgstate-plan` advances to soversion 2;
- `libpkgstate-apply` advances to soversion 1; and
- canonical generation storage becomes `libpkgstate-generation-v2`.

This release contains no in-place migration path. Historical database
and version-1 generation import belong to a separate explicit tool that
must publish a fresh native state target with complete admitted
authority.

## 0.6.0

Last release of the pre-native model. It is not ABI or storage compatible
with 1.0.0.
