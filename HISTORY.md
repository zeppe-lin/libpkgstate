# History

## 1.0.0

Native installed-state authority reset.

- Removed the historical CRUX database model, backend, mutable transaction, and
  import API from the authoritative library.
- Replaced incomplete installed control with complete source, reason, and build
  provenance.
- Added source-authoritative package records and typed runtime/lifecycle facts.
- Added native installation receipts and rich ownership/reconciliation records.
- Advanced snapshot, ownership, publication, and generation formats.
- Added the `libpkgstate-source` adapter for libpkgsource 1.0.0.
- Reset `libpkgstate-plan` to native complete-state projection.
- Reset `libpkgstate-apply` to require explicit incoming source/build authority.
- Removed the compatibility `pkginfo` frontend; retained canonical read-only
  `pkgstate-check`.

ABI decisions:

- core `libpkgstate` advances to soversion 2;
- `libpkgstate-source` begins at soversion 1;
- `libpkgstate-plan` advances to soversion 2; and
- `libpkgstate-apply` advances to soversion 1.

Storage format advances to `libpkgstate-generation-v2`.

## 0.6.0

Last release of the pre-native model. It is not ABI or storage compatible with
1.0.0.
