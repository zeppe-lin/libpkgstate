# Migration boundary

`libpkgstate` 1.0.0 does not read or write the historical CRUX package database.
It does not contain a compatibility model or a legacy import API.

A future standalone importer must be treated as an observation and admission
program, not as a backend mode. It must:

1. read the historical database without modifying it;
2. identify exactly which facts are retained and which are absent;
3. require explicit package release, source, build, lifecycle, architecture,
   installation-reason, target, and application evidence for absent facts;
4. construct complete native installation receipts or a separately reviewed
   bootstrap contract;
5. publish only into a fresh native state target; and
6. emit durable migration evidence.

The importer must never split opaque old version lines by guesswork, substitute
current recipes for historical source authority, or infer package meaning from
archive filenames.
