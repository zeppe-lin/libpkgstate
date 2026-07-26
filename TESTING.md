# Qualification

The repository test suite covers:

- identity wire format and domain separation;
- package/profile/architecture normalization;
- source record, installed control, ownership, and receipt invariants;
- deterministic snapshot and ownership identities;
- publication request and receipt validation;
- stale-safe immutable generation publication and recovery;
- source, planner, and application adapter contracts against their exact public
  APIs;
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
  -Dplanner_adapter=enabled \
  -Dapplication_adapter=enabled \
  -Dtools=enabled \
  -Dwerror=true
meson compile -C build
meson test -C build --print-errorlogs
```

Sanitizer qualification uses address and undefined-behavior sanitizers. Shared
and static dependency closures are built separately.
