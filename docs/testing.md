# Testing

A release is qualified in separate shared and static builds. The suite covers state values, identity derivation, publication projection and codecs, every public header, pkg-config closure, installed consumers, generated manuals, strict Doxygen, release metadata, and repository-boundary contracts.

The architecture contract rejects adapter and concrete-provider headers, sources, options, dependencies, and documentation claims in the core repository. Adapter behavior is tested by the repository that owns the translation.


## Test topology

Tests are separated into `unit`, `integration`, `protocol`, `header`, and
`contract` suites. Fixtures construct canonical state authority; support code
contains only reusable assertions and protocol helpers. Integration tests keep
publication request validation, pure projection, typed receipt construction,
and backend-neutral compare-and-publish behavior independently observable.
Protocol tests separately qualify generation records, publication requests, and
publication receipts so wire failures do not collapse into model failures.

Generation qualification includes empty state and a rich state containing
profile-expansion provenance, lifecycle control, all installed object classes,
hard-link and rejected-object evidence, and transaction provenance.

## Public documentation contract

The source contract checks file, umbrella, enum, macro, and ownership
coverage without requiring a documentation generator. The CI documentation
lane additionally runs `tools/check-doxygen-contract.py` with Clang's parsed
comment AST before invoking Doxygen. That gate requires every public function,
friend operator, constructor, and accessor to document every named parameter
and every non-void return value. Doxygen then remains the authoritative
renderer and final warnings-as-errors check.

## ABI manifest qualification

The reviewed ELF manifest is C-locale sorted and contains only mangled symbols
whose identity carries the `pkgstate` namespace. The version-script generator
rejects foreign implementation exports and non-canonical ordering at configure
time. Shared-library qualification independently extracts the dynamic surface
with `nm`, normalizes it under the C locale, and compares it byte-for-byte with
the reviewed manifest.
