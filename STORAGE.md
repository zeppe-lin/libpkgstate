libpkgstate storage contract
============================

Canonical compare-and-publish
-----------------------------

`pkgstate::canonical_store` exposes complete canonical state:

```cpp
snapshot read() const;
state_publication_receipt
compare_and_publish(const state_publication_request&) const;
```

`compare_and_publish()` is non-virtual and owns the publication sequence. It:

1. obtains one exclusive `canonical_publication_transaction`;
2. reads the actual complete snapshot held by that transaction;
3. rejects a target-binding mismatch as backend corruption;
4. returns a stale receipt without mutation when snapshot identities differ;
5. derives the only snapshot permitted by the request deltas;
6. invokes the backend publication primitive exactly once; and
7. maps the constrained backend result into a typed receipt.

The transaction constructor acquires the publication lock and performs the
authoritative reread under that lock. Its destructor releases the lock. The
backend receives the already validated resulting snapshot; it cannot replace it
with a caller-authored state value through this interface.

`state_publication_backend_result` can report confirmed publication, semantic
rejection, failure before publication, publication without durable confirmation,
or an indeterminate attempt. It cannot report stale state. Staleness is decided
only by the non-virtual comparison layer. Publication outcomes that crossed a
storage boundary require an explicit atomicity boundary.

Failure to acquire the lock or obtain a trustworthy actual snapshot raises
`store_error`, because no receipt can truthfully cite an observed prior state.
Once prior state is known, ordinary attempt outcomes are returned as receipts.

Compatibility store abstraction
-------------------------------

`pkgstate::store` currently exposes the historical storage interface:

```cpp
legacy_snapshot read() const;
std::unique_ptr<write_transaction> begin_write() const;
```

This interface is explicitly compatibility-scoped.  It reads and mutates
`legacy_snapshot` values only.  It does not publish canonical `snapshot` values
and does not implement stale-safe compare-and-publish.

The canonical model provides immutable `state_publication_request` values, and
`canonical_store` now enforces the compare-and-publish sequence. A concrete
canonical backend must still preserve complete records without loss and
implement the lock-scoped publication transaction.

`read()` returns one immutable observation of compatibility records.
`begin_write()` returns an isolated transaction initialized from current
durable compatibility state.

Compatibility write transaction
-------------------------------

A transaction owns a private mutable `legacy_installed_package` set until
`commit()` succeeds.

Operations:

```text
current()    return the uncommitted legacy snapshot
put()        add or replace one legacy package by name
erase()      remove one legacy package by name
commit()     replace the complete compatibility database
committed()  report successful compatibility publication
```

`put()` cannot accept canonical `installed_package`.  The type boundary blocks
silent rich-to-legacy down-conversion before serialization.

`erase()` validates the supplied name using the historical package-identity
representation rules.  It returns false when no record existed.

After successful commit, further mutation or commit attempts are invalid and
raise `transaction_error`.  Destroying an uncommitted transaction discards its
uncommitted package set.

No canonical or filesystem transaction
--------------------------------------

The compatibility transaction does not make package deployment atomic.

A higher layer may perform:

```text
stage payloads
mutate installed filesystem
publish package state
run maintenance
```

The interface protects only the historical database replacement.  It performs
no caller-supplied expected-snapshot comparison and is not the canonical
publication protocol.  Failure before or after state publication remains the
responsibility of the complete package transaction engine.

Legacy text format
------------------

`legacy_text_store` reads and writes the line-oriented database conventionally
located at:

```text
/var/lib/pkg/db
```

The constructor accepts an explicit database pathname.  It does not prepend a
root directory itself.  The exact format is documented in `pkgstate-db(5)`.

The backend can retain only:

```text
package name
opaque version line
directory or non-directory ownership paths
```

It therefore constructs `legacy_installed_package` and `legacy_snapshot`, not
canonical installed packages and snapshots.  It does not derive canonical
release, control, target, evidence, or identity facts from current inputs.

Locking
-------

The backend opens the database parent directory and applies non-blocking
advisory `flock(2)` locking:

```text
read()          shared lock
begin_write()   exclusive lock
```

The write lock remains held until:

* commit completes;
* the transaction is destroyed; or
* transaction construction fails.

Lock contention raises `store_error`; the call does not wait.

The lock is advisory.  Programs that bypass the contract and edit the database
without taking the same directory lock can race with readers and writers.

Read contract
-------------

A read:

1. acquires the shared directory lock;
2. opens the database as binary input;
3. parses every package record;
4. validates historical identities and canonical paths;
5. constructs incomplete compatibility package records; and
6. returns one indexed `legacy_snapshot`.

Malformed input raises `store_error` with the database pathname and line number
where available.

Publication contract
--------------------

Commit publication is a complete compatibility-state replacement:

1. create a same-directory temporary file;
2. set its mode to `0444`;
3. serialize the complete compatibility transaction snapshot;
4. write all bytes;
5. synchronize the temporary file with `fsync(2)`;
6. close the temporary file;
7. remove any previous `db.backup`;
8. hard-link the current database to `db.backup` when it exists;
9. rename the temporary file over the database; and
10. synchronize the database directory.

A same-directory temporary file and rename preserve the single-filesystem
rename contract.

The backup is the previous database inode.  It is not a second independently
serialized snapshot.  If the database did not exist, backup creation is
skipped.

Failure interpretation
----------------------

Failure before the rename leaves the previous database pathname authoritative.
The temporary file is removed when possible.

Failure after the rename may mean the new database is visible even though
directory synchronization failed.  The backend reports this explicitly as:

```text
state published but directory sync failed
```

A caller receiving that failure must not assume the old state remained active.
It should inspect or reread durable compatibility state before deciding
recovery.

Permissions
-----------

The compatibility writer publishes the database with mode `0444`.

The backend does not change ownership of the database.  The effective caller,
directory permissions, mount semantics, and system policy determine ownership
and whether publication is permitted.

Alternate roots
---------------

Root selection belongs to the consumer.

For example, `pkginfo -r /mnt/root` constructs the database pathname:

```text
/mnt/root/var/lib/pkg/db
```

The library itself receives that explicit pathname and does not interpret the
alternate root further.

Typed receipt boundary
----------------------

`state_publication_receipt` is the immutable result vocabulary used by the
canonical compare-and-publish interface. It distinguishes stale comparison,
semantic rejection,
failure before publication, confirmed publication, publication without durable
confirmation, and an indeterminate outcome that requires an authoritative
reread.

The receipt factories validate coherent state facts and compute receipt
identity. They do not acquire locks or publish state. The non-virtual
`canonical_store::compare_and_publish()` method constructs receipts from the
actual lock-scoped state and backend result; a caller-created value is not
independent proof that publication occurred.

Canonical backend boundary
--------------------------

The compatibility store is not the canonical state backend.

A canonical backend must preserve complete `snapshot` values, installed
control, target binding, typed identities, publication requests, and receipts
without loss. It implements the `canonical_store` transaction hook and a
lossless storage format. The historical transaction API remains available only
for the old
database during migration.
