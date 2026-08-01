# libpkgstate

`libpkgstate` is the native installed-package state authority for Zeppe-Lin.
Version 2.5 retains the generation-v3 installed-state representation introduced
in 2.0 and adds canonical durable encodings for immutable state-publication
requests and receipts. The codecs reopen evidence only under exact snapshot and
request authority; they do not turn stored identities into semantic bodies.

The library records complete immutable installed truth:

- exact source-authoritative package release, profile, recipe, and source
  snapshot identities;
- package metadata, runtime requirements, lifecycle programs, and
  action-specific lifecycle requirements;
- declared and selected build/target architectures;
- installation reason and complete source-bound native build provenance;
- completed ownership with mode, owner, size, timestamp, content, link, device,
  hard-link, retained-object, and rejected-object evidence;
- installation receipts and installed packages;
- target-bound ownership and installed snapshots; and
- immutable publication requests, receipts, and generations; and
- canonical request and receipt encodings for restart-safe state publication.

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
libpkgbuild successful result + exact libpkgimage inspection
        |
        v
libpkgstate-build -> build_authority
        |
caller-held libpkgapply 2.1 target mutation lease
        |
canonical_store -- one read --> libpkgstate-apply
        |                           |
        |                           v
        |                 lease-bound application state
        |
libpkgapply request-bound incoming build + completed effects
        |
        v
libpkgstate-apply -> source/build admission -> installation_receipt
                  -> publication request
        |
        v
canonical_store -> immutable state generation
```

`libpkgstate-plan` provides the reverse projection needed by the current
operation planner. It exposes only planner-owned runtime/removal/target facts;
it does not flatten state-specific source, build, or application provenance into
planner control.

## Native model

A `package_source_record` is the durable projection of one sealed source
snapshot. It retains source identities rather than recomputing them.

A `build_authority` is admitted only from a complete successful native build
result whose exact artifact bytes and normalized payload have been independently
verified. Its `build_provenance` retains source material, materialized input,
environment, build-policy, request, result, payload, artifact, execution, image,
and inspection identities.

An `installed_control` binds one source record, one typed installation reason,
and build provenance that names that same source record.

Before application, `libpkgstate-apply` reads the canonical store exactly once
while the caller's target mutation lease is live. It returns the snapshot and
its exact accepted-plan path-owner projection as one inseparable value and
derives projection evidence rather than accepting an identity from the caller.

`libpkgstate-apply` also receives the exact operation-specific libpkgapply request,
completed application evidence, and expected native snapshot. For installation
it also receives only the initial installation reason. It derives source and
build authority from the request-bound incoming package, preserves the prior
reason on upgrade, and accepts no incoming authority on removal. Additive
overloads carry one exact transaction-evidence identity into the publication
request and, for install or upgrade, the durable installation receipt.

An `installation_receipt` binds installed control to one target, one completed
ownership manifest, one operation plan, one application-evidence identity, and
optional exact transaction evidence.
`installed_package` can be constructed only from that complete receipt.

A `snapshot` is complete installed package and ownership state for one exact
`state_target_binding`. State-owned identities are derived with domain-separated
canonical records; external identities remain typed references.

## Publication

`state_publication_request` expresses install, replace, and remove deltas against
one expected snapshot. `canonical_store::compare_and_publish()` owns stale-state
comparison and derives the resulting snapshot. Backends cannot silently rebase a
request or accept a caller-authored replacement state.

`encode_state_publication_request()` retains the complete delta bodies but not the
expected snapshot authority. Decode requires that exact snapshot. Publication
receipt decode likewise requires the exact request and actual prior snapshot;
any resulting snapshot is derived from those bodies rather than accepted from
record bytes as replacement state.

`canonical_generation_store` persists complete immutable generations and
atomically selects one current generation. The native storage identifier is
`libpkgstate-generation-v3`.

## Migration boundary

The authoritative library contains no historical database parser, compatibility
snapshot, mutable old-format transaction, or import entry point. It also does
not reinterpret generation-v1 or generation-v2 bytes as generation-v3.
Migration belongs to a separate program that must supply every native fact an
older format did not retain.

## Build

```sh
meson setup build \
  -Ddefault_library=shared \
  -Dlink_mode=shared \
  -Dsource_adapter=enabled \
  -Dbuild_adapter=enabled \
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
- `pkgstate_build_adapter(3)`;
- `pkgstate_publication(3)`;
- `pkgstate_canonical_generation_store(3)`; and
- `pkgstate-generation(5)`.

License: GPL-3.0-or-later.
