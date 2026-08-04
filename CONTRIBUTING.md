# Contributing

Changes must preserve the pure-owner boundary documented in `docs/architecture.md`.

Do not add source parsing, build admission, image inspection, planning projection, application admission, target mutation, migration, compatibility import, or orchestration to `libpkgstate`. A translation against another semantic owner belongs in the exact role-qualified adapter repository.

For a semantic change:

1. identify the state-owned invariant being changed;
2. analyze identity, ABI, publication-record, and generation-storage consequences independently;
3. add focused construction, refusal, corruption, and restart tests as applicable;
4. update public declarations, manuals, design, storage, migration, and history together;
5. review shared and static dependency closure;
6. make the version, SONAME, evidence schema, and storage-version decision explicit.

State values are immutable and authority is passed explicitly. Diagnostic text is not control flow. The library must not consult ambient source trees, package archives, target filesystems, current recipes, or planner state to repair missing durable facts.

Use C++17, POSIX shell for contract programs, GPL-3.0-or-later SPDX headers, and the checked-in formatting policy. Keep commits single-purpose and suitable for `git am` review.
