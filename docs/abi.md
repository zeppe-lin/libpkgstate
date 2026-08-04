# ABI and durable protocol policy

`libpkgstate` 3.0.0 advances the core SONAME from 3 to 4 because the exported
`canonical_generation_store` mechanism moves to `libpkgstate-posix`. The core
also gives `canonical_store` and `canonical_publication_transaction` stable
out-of-line virtual destructors and publishes the generation-v4 record codec as
state-owned protocol authority.

The exact reviewed ELF surface is stored in `abi/libpkgstate.exports`. Shared
builds use hidden visibility and a generated version script. The 3.0 surface
contains only symbols whose mangled identity carries the `pkgstate` namespace;
pure libstdc++ implementation instantiations remain local to the shared object.
Export additions, removals, signature changes, exception hierarchy changes, or
public value-layout changes require an explicit ABI decision.

The pkg-config surface has no public requirements and one private implementation
requirement: `libcrypto`. Ordinary shared-consumer flags must not expose crypto;
the static closure must include it.

Four version axes are independent:

1. repository and source release;
2. C++ ABI and SONAME;
3. durable publication-evidence protocols; and
4. canonical generation records and independently released storage mechanisms.

Release 3.0 extracts adapters and the concrete generation provider. It advances
the core C++ SONAME, preserves publication-evidence schema 2, and advances
canonical generation records to version 4 under one state-owned encoder and
decoder. `libpkgstate-posix` consumes those bytes without owning a competing
codec. Generation-v3 stores require explicit migration because source recipe
identity was retired by `libpkgsource` 3.0 and is no longer part of the durable
source record.
Future work must not infer one version decision from another.
