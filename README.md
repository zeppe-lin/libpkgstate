libpkgstate
===========

`libpkgstate` is a C++17 library for representing durable installed package
state and querying package ownership.

The implementation is original Zeppe-Lin code.  It is not derived from CRUX
`pkgutils` or from the former CRUX-derived implementation in `libpkgcore`.

This first model layer provides validated package identities, canonical sorted
installed manifests with directory identity, immutable snapshots, package
lookup, and shared ownership queries.  Persistent storage is added in the
following commit.
