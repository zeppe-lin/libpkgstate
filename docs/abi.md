# ABI and durable protocol policy

`libpkgstate` 3.0.0 advances the core SONAME from 3 to 4 because the exported
`canonical_generation_store` class moves to `libpkgstate-posix`. The reviewed
ELF export set is stored in `abi/libpkgstate.exports`; shared builds use hidden
visibility and a generated version script. Export additions, removals, signature
changes, exception hierarchy changes, or public value-layout changes require an
explicit ABI decision.

The pkg-config surface has no public requirements and one private implementation
requirement: `libcrypto`. Ordinary shared-consumer flags must not expose crypto;
the static closure must include it.

Four version axes are independent:

1. repository and source release;
2. C++ ABI and SONAME;
3. durable publication-evidence protocols; and
4. independently released storage-provider protocols.

Release 3.0 extracts adapters and the concrete generation provider. It advances
only the core C++ SONAME, preserves publication-evidence schema 2, and preserves
generation-v3 bytes in `libpkgstate-posix`. Future work must not infer one
version decision from another.
