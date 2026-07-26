# libpkgstate

`libpkgstate` is the native installed-package state authority for Zeppe-Lin.
Version 1 is intentionally incompatible with the former CRUX-shaped model.

The library records complete immutable installed truth:

- exact source-authoritative package release, profile, recipe, and source
  snapshot identities;
- package metadata, runtime requirements, lifecycle programs, and
  action-specific lifecycle requirements;
- declared and selected build/target architectures;
- installation reason and complete build provenance;
- completed ownership with mode, owner, size, timestamp, content, link, device,
  hard-link, retained-object, and rejected-object evidence;
- installation receipts and installed packages;
- target-bound ownership and installed snapshots; and
- immutable publication requests, receipts, and generations.

The core library does not parse recipe syntax, inspect archives, build packages,
resolve dependencies, execute lifecycle programs, mutate target filesystems, or
import historical package databases.

## Authority flow

```text
libpkgsource sealed snapshot
        |
        v
libpkgstate-source -> package_source_record
        |
resolved build and artifact identities
        |
completed libpkgapply evidence + accepted libpkgplan operation
        |
        v
libpkgstate-apply -> installation_receipt -> publication request
        |
        v
canonical_store -> immutable state generation
```

`libpkgstate-plan` provides the reverse projection needed by the current
operation planner. It exposes only planner-owned runtime/removal/target facts;
it does not flatten state-specific provenance into planner control.

## Native model

A `package_source_record` is the durable projection of one sealed source
snapshot. It retains source identities rather than recomputing them.

An `installed_control` adds one typed installation reason and build provenance:
candidate control, resolved build-input set, build result, artifact, and artifact
manifest.

An `installation_receipt` binds installed control to one target, one completed
ownership manifest, one operation plan, and one application-evidence identity.
`installed_package` can be constructed only from that complete receipt.

A `snapshot` is complete installed package and ownership state for one exact
`state_target_binding`. State-owned identities are derived with domain-separated
canonical records; external identities remain typed references.

## Publication

`state_publication_request` expresses install, replace, and remove deltas against
one expected snapshot. `canonical_store::compare_and_publish()` owns stale-state
comparison and derives the resulting snapshot. Backends cannot silently rebase a
request or accept a caller-authored replacement state.

`canonical_generation_store` persists complete immutable generations and
atomically selects one current generation. The native storage identifier is
`libpkgstate-generation-v2`.

## Migration boundary

The authoritative library contains no historical database parser, compatibility
snapshot, mutable old-format transaction, or import entry point. Migration from
`/var/lib/pkg/db` will be implemented later as a separate program that must
supply every native fact the old format did not retain.

## Build

```sh
meson setup build \
  -Ddefault_library=shared \
  -Dlink_mode=shared \
  -Dsource_adapter=enabled \
  -Dplanner_adapter=enabled \
  -Dapplication_adapter=enabled
meson compile -C build
meson test -C build --print-errorlogs
```

Core requires C++17 and libcrypto. Optional adapters require their exact owning
libraries. Shared and static builds are configured separately.

## Documentation

The normative contracts are documented in:

- `pkgstate_authority(7)`;
- `pkgstate_model(3)`;
- `pkgstate_installation_receipt(3)`;
- `pkgstate_publication(3)`;
- `pkgstate_canonical_generation_store(3)`; and
- `pkgstate-generation(5)`.

License: GPL-3.0-or-later.
