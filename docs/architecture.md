# Architecture

## Authority

`libpkgstate` owns complete immutable installed-package state for one exact target binding. It owns state-domain identities, publication requests and receipts, publication evidence codecs, canonical generation-v4 binding and snapshot records, and compare-and-publish semantics.

The library accepts already admitted facts. It does not parse source syntax, execute builds, inspect package images, plan operations, execute application, inspect a target filesystem, or acquire mutation leases.

## Extracted translation boundaries

The 3.0 generation removes all foreign-authority adapters from the state owner:

- `libpkgstate-source` translates sealed source authority into `package_source_record`;
- `libpkgstate-build` admits successful build and image authority;
- `libpkgstate-plan` projects installed state into planner facts;
- `libpkgstate-apply` reads lease-bound state projections and admits completed application evidence.

Those repositories depend inward on `libpkgstate`. The state owner depends on none of them.

## Persistence placement

The core owns publication evidence codecs, canonical generation-v4 record bytes, and the non-virtual compare-and-publish sequence. Concrete paths, directories, locks, selectors, durability, recovery, and diagnostic mechanisms are provider authority. `libpkgstate-posix` persists the state-owned records in the reference immutable-generation layout and depends inward on this core; the core does not depend outward on it.

## Forbidden dependencies

The core build, public headers, pkg-config metadata, and dynamic linkage must not mention `libpkgsource`, `libpkgbuild`, `libpkgimage`, `libpkgplan`, `libpkgapply`, or any `libpkgstate-*` adapter or provider.
