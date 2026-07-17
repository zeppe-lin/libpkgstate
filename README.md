libpkgstate
===========

`libpkgstate` is a C++17 library for representing and persisting durable
installed package state.

It provides validated package identities, canonical ownership manifests,
immutable indexed snapshots, backend-neutral write transactions, and an
original compatibility backend for the historical `/var/lib/pkg/db` format.

`libpkgstate` records installed-state truth.  It does not inspect archives,
apply install policy, mutate the live filesystem, stage rejected files, or
coordinate complete package transactions.

The implementation is original Zeppe-Lin code.  It is not derived from CRUX
`pkgutils` or from the former CRUX-derived implementation in `libpkgcore`.
