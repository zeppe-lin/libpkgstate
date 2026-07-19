libpkgstate storage contract
============================

Store abstraction
-----------------

`pkgstate::store` exposes two operations:

```cpp
snapshot read() const;
std::unique_ptr<write_transaction> begin_write() const;
```

A store is responsible for durable package-state storage only.

`read()` returns one immutable snapshot.  `begin_write()` returns an isolated
transaction initialized from the current durable snapshot.

Write transaction
-----------------

A transaction owns a private mutable package set until `commit()` succeeds.

Operations:

```text
current()    return the uncommitted snapshot
put()        add or replace one package by name
erase()      remove one package by name
commit()     publish the complete current snapshot
committed()  report successful publication
```

`put()` replaces an existing record with the same package name.

`erase()` validates the supplied name using the package-identity
representation rules.  It returns false when no record existed.

After successful commit, further mutation or commit attempts are invalid and
raise `transaction_error`.

Destroying an uncommitted transaction discards its uncommitted package set.

No filesystem transaction
-------------------------

The state transaction does not make package deployment atomic.

A higher layer may perform:

```text
stage payloads
mutate installed filesystem
publish package state
run maintenance
```

`libpkgstate` coordinates only the publication step.  Failure before or after
that step remains the responsibility of the complete package transaction
engine.

Legacy text format
------------------

`legacy_text_store` reads and writes the line-oriented database conventionally
located at:

```text
/var/lib/pkg/db
```

The constructor accepts an explicit database pathname.  It does not prepend a
root directory itself.

The exact format is documented in `pkgstate-db(5)`.

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
4. validates identities and paths;
5. constructs canonical installed packages; and
6. returns one indexed snapshot.

Malformed input raises `store_error` with the database pathname and line
number where available.

Publication contract
--------------------

Commit publication is a complete-state replacement:

1. create a same-directory temporary file;
2. set its mode to `0444`;
3. serialize the complete transaction snapshot;
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
serialized snapshot.

If the database did not exist, backup creation is skipped.

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
It should inspect or reread durable state before deciding recovery.

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

Backend migration
-----------------

The compatibility format is one `store` implementation, not the installed
state model itself.

A future backend must preserve the public model invariants:

* validated identities;
* canonical ownership paths;
* directory versus non-directory identity;
* empty manifests;
* duplicate rejection;
* shared ownership;
* deterministic snapshots; and
* isolated complete-state publication.

It need not preserve the text encoding, directory locking mechanism, or backup
pathname unless interoperability requires them.
