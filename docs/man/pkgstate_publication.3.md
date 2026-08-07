% PKGSTATE_PUBLICATION(3) libpkgstate | Version 3.0.0

<!-- Generated from pkgstate_publication.3.scdoc; do not edit. -->


# NAME

pkgstate_publication - immutable installed-state transition contracts

# REQUEST

**state_publication_request::make()** accepts one expected **snapshot** and one or
more package deltas. A delta installs, replaces, or removes one package and binds
its operation-plan and application-evidence identities. A composed request must
also carry transaction evidence.

The request validates target consistency, package expectations, duplicate names,
and operation shape. It is immutable and performs no storage I/O.

# PURE PROJECTION

**project_publication_request()** applies one exact request to its complete
actual-prior **snapshot**. It rejects a foreign target, stale prior epoch, or
contradictory package delta and returns the only valid resulting immutable
snapshot. It performs no store access, target observation, publication, retry,
repair, or mutation.

# COMPARE AND PUBLISH

**canonical_store::compare_and_publish()** owns the stale-state check. Under the
backend publication boundary it reads the actual prior snapshot and:

. returns **stale_expected_state** without invoking publication when the expected snapshot differs;
. derives the resulting snapshot from request deltas when the expectation is current; and
. asks the backend to publish that exact result.

The backend cannot silently rebase an old request or accept a caller-authored
replacement snapshot.

# DURABLE REQUEST EVIDENCE

**encode_state_publication_request()** records exact delta bodies, request identity,
expected-snapshot identity, target binding, and optional transaction evidence.
**decode_state_publication_request()** requires the complete expected **snapshot**.
It does not recreate that snapshot from its identity or read a canonical store.

Proposed installed packages are retained as complete native state bodies. Decode
rebuilds each install, replacement, or removal delta through the public
invariant-enforcing factories and requires canonical byte-for-byte re-encoding.

Request records use fixed eight-byte magic **ZLSPRQST** followed by
big-endian schema version 1. The decoder accepts only this canonical framing.

# RECEIPT

**state_publication_receipt** records request, expected and actual prior snapshots,
target binding, storage format, outcome, durability, atomicity boundary,
optional resulting snapshot, and subordinate evidence.

A publication receipt does not prove filesystem transaction atomicity. It proves
only the state-storage outcome and boundary explicitly recorded in the receipt.

# DURABLE RECEIPT EVIDENCE

**encode_state_publication_receipt()** records the exact request, expected and
actual-prior snapshot identities, target binding, storage format, outcome,
durability, atomicity, optional result, and subordinate evidence.
**decode_state_publication_receipt()** requires the exact request and complete
actual-prior **snapshot**.

For published and durability-unconfirmed outcomes, decode derives the only
possible resulting snapshot from the request and actual prior. An indeterminate
receipt may cite that derived identity or no result. The codec never accepts a
caller-authored replacement snapshot, opens storage, invokes a backend, retries,
repairs, or publishes state.

Receipt records use fixed eight-byte magic **ZLSPRCPT** followed by
big-endian schema version 1. The decoder accepts only this canonical framing.

# OUTCOMES

Published, stale expected state, rejected request, failure before publication,
published with unconfirmed durability, and indeterminate outcome are distinct
typed states.

# SEE ALSO

**pkgstate_store**(3), **libpkgstate-posix**(3), **pkgstate-generation**(5)
