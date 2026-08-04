# ABI and durable protocol policy

`libpkgstate` 3.0.0 preserves core SONAME generation 3. The reviewed ELF export set is stored in `abi/libpkgstate.exports`; shared builds use hidden visibility and a generated version script. Export additions, removals, signature changes, exception hierarchy changes, or public value-layout changes require an explicit ABI decision.

The pkg-config surface has no public requirements and one private implementation requirement: `libcrypto`. Ordinary shared-consumer flags must not expose crypto; the static closure must include it.

Three version axes are independent:

1. the repository and source release;
2. the C++ ABI and SONAME;
3. durable publication-evidence and generation-storage protocols.

Release 3.0 changes repository ownership by extracting adapters. It preserves core SONAME 3 and publication evidence schema 2. Generation-v3 storage continues in the independently released `libpkgstate-posix` provider. Future work must not infer one version decision from another.
