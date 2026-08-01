# Qualification

The repository test suite covers:

- identity wire format and domain separation;
- package/profile/architecture normalization;
- source record, source-bound build provenance, installed control, ownership,
  and receipt invariants;
- exact build-result, artifact-byte, image-inspection, and payload admission;
- deterministic snapshot and ownership identities;
- publication request and receipt validation;
- stale-safe immutable generation publication and recovery;
- source, build, planner, and application adapter contracts against their exact
  public APIs, including request-bound source/build derivation and rejection of
  foreign incoming artifacts;
- additive transaction-provenance projection for installation, upgrade, and
  removal, including exact receipt/publication agreement and preservation of
  the existing no-evidence identities;
- public header independence;
- generated pkg-config metadata;
- read-only diagnostic behavior;
- release and documentation source contracts; and
- installed shared/static consumers in CI.

Run:

```sh
meson setup build \
  -Ddefault_library=shared \
  -Dlink_mode=shared \
  -Dsource_adapter=enabled \
  -Dbuild_adapter=enabled \
  -Dplanner_adapter=enabled \
  -Dapplication_adapter=enabled \
  -Dtools=enabled \
  -Dwerror=true
meson compile -C build
meson test -C build --print-errorlogs
```

Sanitizer qualification uses address and undefined-behavior sanitizers. Shared
and static dependency closures are built separately.

- lease-bound application-state projection performs one canonical read, rejects
  absent, lost, foreign, stale, or target-mismatched authority, derives stable
  evidence, and never enters publication;
