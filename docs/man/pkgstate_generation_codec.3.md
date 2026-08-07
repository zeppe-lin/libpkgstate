% PKGSTATE_GENERATION_CODEC(3) libpkgstate | Version 3.1.0

<!-- Generated from pkgstate_generation_codec.3.scdoc; do not edit. -->


# NAME

pkgstate_generation_codec - canonical generation-v1 state records

# SYNOPSIS

**#include <libpkgstate/generation_codec.h>**

# DESCRIPTION

The generation codec is the **libpkgstate**-owned binary protocol for one exact
**state_target_binding** and one complete native **snapshot**. It performs no I/O
and owns no storage path, lock, selector, recovery, or durability policy.

**encode_generation_binding()** and **encode_generation_snapshot()** emit canonical
version-1 records. **decode_generation_binding()** and
**decode_generation_snapshot()** validate framing, version, normalized value
shape, target binding, state-owned identities, and canonical re-encoding.
Malformed, unsupported, non-canonical, or identity-inconsistent records raise
**store_error**.

# VERSION

The record version is **canonical_generation_storage_version**, currently 1. Its
receipt-visible identifier is **canonical_generation_storage_format**, currently
**libpkgstate-generation-v1**.

Repository release, C++ SONAME, publication-evidence schema, and generation
record version are independent version axes.

# PROVIDERS

Storage providers persist these bytes without reinterpreting them.
**libpkgstate-posix**(3) provides the reference immutable-generation filesystem
mechanism.

# SEE ALSO

**libpkgstate**(3), **pkgstate_store**(3), **libpkgstate-posix**(3),
**pkgstate-generation**(5)
