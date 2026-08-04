# Testing

A release is qualified in separate shared and static builds. The suite covers state values, identity derivation, publication codecs, every public header, pkg-config closure, installed consumers, generated manuals, strict Doxygen, release metadata, and repository-boundary contracts.

The architecture contract rejects adapter and concrete-provider headers, sources, options, dependencies, and documentation claims in the core repository. Adapter behavior is tested by the repository that owns the translation.


## Public documentation contract

The source contract checks file, umbrella, enum, macro, and ownership
coverage without requiring a documentation generator. The CI documentation
lane additionally runs `tools/check-doxygen-contract.py` with Clang's parsed
comment AST before invoking Doxygen. That gate requires every public function,
friend operator, constructor, and accessor to document every named parameter
and every non-void return value. Doxygen then remains the authoritative
renderer and final warnings-as-errors check.
