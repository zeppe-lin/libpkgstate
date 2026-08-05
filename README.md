# libpkgstate

`libpkgstate` is the native durable installed-package state authority for Zeppe-Lin package management.

It answers one question:

```text
what complete package state is durably installed for this exact managed target?
```

## Owned authority

The library owns immutable state-domain values and their identities:

- exact target binding and canonical snapshot epochs;
- package releases, source records, build provenance, installed control, installation receipts, installed packages, and complete ownership;
- state-publication requests and receipts with stale-safe compare-and-publish semantics;
- canonical publication evidence encoding;
- canonical generation-v1 target-binding and complete-snapshot records; and
- the backend-neutral stale-safe `canonical_store` contract.

There is no incomplete native installed package. Construction requires complete source, build, application, target, ownership, plan, and receipt evidence as represented by the state model. Missing historical facts are migration input, not optional fields silently filled from current ambient state.

## Boundary

The core accepts already admitted state values. It does not parse source syntax, select architectures, execute builds, inspect archives, resolve dependencies, plan operations, execute lifecycle programs, mutate target filesystems, acquire application leases, or import historical databases.

Foreign-authority translations live in independent repositories:

```text
libpkgsource ------> libpkgstate-source --+
                                           |
libpkgbuild/image -> libpkgstate-build -----+--> libpkgstate values
                                           |
libpkgstate -------> libpkgstate-plan ------+--> libpkgplan facts
                                           |
libpkgapply -------> libpkgstate-apply -----+--> publication request
```

Those repositories depend inward on `libpkgstate`; the owner depends on none of them. The core build, installed headers, pkg-config metadata, and dynamic linkage have one external implementation dependency: OpenSSL `libcrypto`, used privately for qualified SHA-256 operations. They contain no concrete storage provider.

Canonical generation-v1 record bytes are a state-owned durable protocol
exposed by `generation_codec.h`. Concrete persistence mechanisms are
independent providers. The reference filesystem layout, locking, selector
replacement, durability, recovery refusal, and read-only diagnostics live in
`libpkgstate-posix`; the state owner does not select, open, or depend on that
provider.

## Native model

A `package_source_record` retains the source-owned release, selected-profile,
and complete source-snapshot identities together with state-relevant metadata,
runtime requirements, lifecycle authority, and selected architecture binding.
The 3.0 source owner no longer issues a separate recipe identity.

`build_provenance` retains the logical request and resolver-backed input set, environment and build policies, successful result, payload, artifact, exact artifact bytes, execution evidence, one admitted build-to-image binding, the normalized image, and independent inspection identities. `installed_control` binds that provenance to its exact source record and typed installation reason.

An `installation_receipt` binds installed control to one target, complete object ownership, one accepted operation plan, completed application evidence, and optional exact transaction evidence. `installed_package` exists only from a complete receipt.

A `snapshot` is the complete package and ownership state for one `state_target_binding`. State-owned identities are derived from domain-separated canonical records; external identities remain typed references and are never re-derived from coordinates or filenames.

## Publication

Callers submit deltas against one expected snapshot. `canonical_store::compare_and_publish()` reads the actual prior state, refuses stale authority before backend publication, and derives the only admissible resulting snapshot. A backend cannot silently rebase a request or accept a caller-authored replacement state.

Publication request decode requires the exact expected snapshot. Receipt decode requires the exact request and actual prior snapshot. Durable records may retain identities and complete delta bodies, but they never promote a digest string into missing semantic authority.

Request and receipt encoders emit the first canonical schema with fixed eight-byte house framing:

```text
ZLSPRQST  state-publication request
ZLSPRCPT  state-publication receipt
```


## Storage providers

`canonical_store` owns the backend-neutral stale-safe publication sequence.
The canonical generation codec owns the portable binding and complete-snapshot
record bytes used by publication evidence and storage providers. A concrete
provider owns storage layout, locking, selector publication, durability,
recovery, and diagnostics. The reference filesystem provider is
`libpkgstate-posix`; this repository does not link or instantiate it.

Historical import remains a separate observation and admission problem described
in `MIGRATION.md`.

## Build

```sh
meson setup build-shared \
  -Ddefault_library=shared \
  -Dlink_mode=shared \
meson compile -C build-shared
meson test -C build-shared --print-errorlogs

meson setup build-static \
  -Ddefault_library=static \
  -Dlink_mode=static \
meson compile -C build-static
meson test -C build-static --print-errorlogs
```

Shared and static closures require separate build directories. Fallback subprojects and `default_library=both` are intentionally unsupported.

## Documentation

- `DESIGN.md` — semantic and publication invariants;
- `MIGRATION.md` — explicit non-native import boundary;
- `TESTING.md` — qualification matrix;
- `docs/architecture.md` — repository ownership;
- `docs/integration.md` — adapter and provider graph;
- `docs/abi.md` — ABI and durable protocol policy;
- `MAINTAINING.md` — release gate.

## License

GPL-3.0-or-later. See `COPYING` and `COPYRIGHT`.
