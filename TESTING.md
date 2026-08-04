# Qualification

The core repository qualifies only state-owned behavior:

- identity wire format and domain separation;
- package, profile, architecture, source-record, build-provenance, installed-control, ownership, receipt, snapshot, and target-binding invariants;
- publication request and receipt validation and durable evidence codecs;
- stale-safe backend-neutral compare-and-publish behavior;
- public-header independence, reviewed ELF exports, SONAME 4, and pkg-config closure;
- manual, Doxygen, documentation, repository, architecture, and release contracts; and
- installed shared and static consumers.

Foreign-authority translation behavior is qualified in `libpkgstate-source`, `libpkgstate-build`, `libpkgstate-plan`, and `libpkgstate-apply`. The owner does not rebuild those adapters behind feature options.

Run one closure per build directory:

```sh
meson setup build \
  -Ddefault_library=shared \
  -Dlink_mode=shared \
  -Dwerror=true
meson compile -C build
meson test -C build --print-errorlogs
```

Sanitizer qualification uses address and undefined-behavior sanitizers. Shared and static closures are built separately. Installed-consumer qualification must confirm that no source, build, image, planning, application, concrete storage provider, or extracted-state-adapter dependency appears in the core pkg-config surface.
