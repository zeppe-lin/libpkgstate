# libpkgstate

`libpkgstate` is the native installed-package state authority for Zeppe-Lin package management.

It owns complete immutable snapshots for one exact target binding, path ownership, installation receipts, source/build/application provenance retained as typed references, durable publication requests and receipts, canonical publication records, compare-and-publish semantics, and the immutable generation-v3 storage backend.

## Boundary

The core has one external implementation dependency: OpenSSL `libcrypto`, used privately for qualified SHA-256 operations. It does not depend on package source, build, image, planning, or application libraries.

Foreign-authority translations live in independent repositories:

- `libpkgstate-source` — sealed source authority to durable source records;
- `libpkgstate-build` — successful build and image evidence to build authority;
- `libpkgstate-plan` — installed state to planner-owned facts;
- `libpkgstate-apply` — lease-bound application projection and completed-application admission.

The publication codec and canonical generation store remain in the owner because they persist state-owned authority rather than translate another subsystem's model.

Read `docs/architecture.md` for placement rules and `docs/integration.md` for the dependency graph.

## Build

```sh
meson setup build   -Ddefault_library=shared   -Dlink_mode=shared
meson compile -C build
meson test -C build --print-errorlogs
```

Shared and static closures use separate build directories. Meson fallback subprojects are intentionally unsupported.

## License

GPL-3.0-or-later. See `COPYING` and `COPYRIGHT`.
