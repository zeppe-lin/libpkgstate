# Migration boundary

`libpkgstate` 2.0.0 does not read or write the historical CRUX package database.
It does not contain a compatibility model or a legacy import API.

Version 2.0.0 also does not perform an in-place upgrade of generation-v1 or
generation-v2 canonical storage. Do not point a 2.0.0 writer at an existing
0.6.0 or 1.0.0 state store and expect reinterpretation or repair. Native
publication begins with a fresh state target.

A future standalone importer must be treated as an observation and admission
program, not as a backend mode. It must:

1. read the historical database or old generation store without modifying it;
2. identify exactly which facts are retained and which are absent;
3. require explicit package release, source, build, lifecycle, architecture,
   installation-reason, target, application, and ownership evidence for absent
   facts;
4. construct complete native installation receipts or a separately reviewed
   bootstrap contract;
5. publish only into a fresh native state target; and
6. emit durable migration evidence.

A generation-v2 importer cannot treat its planner candidate-control and artifact
manifest references as native build authority. It must obtain or explicitly
bootstrap the request, source-material, materialized-input, policy, result,
payload, artifact-content, execution, image, and inspection facts required by
the 2.0.0 model.

The importer must never split opaque old version lines by guesswork, substitute
current recipes for historical source authority, infer package meaning from
archive filenames, or counterfeit absent build/application evidence.
