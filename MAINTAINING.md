# Maintaining libpkgstate

The release gate is the complete owner contract, not merely a green unit-test executable.

Before tagging:

1. confirm the core contains no foreign-authority adapter or dependency;
2. build clean GCC and Clang shared and static configurations separately;
3. run the optimized release and ASan/UBSan configurations;
4. compile every installed public header independently;
5. compare exports with `abi/libpkgstate.exports` and verify SONAME 4;
6. inspect pkg-config and `DT_NEEDED`; only private `libcrypto` is permitted;
7. exercise publication codecs, stale comparison, indeterminate publication, and backend-contract refusal;
8. lint all manuals and run strict Doxygen;
9. stage-install the library, manuals, and project documentation, then compile an installed consumer;
10. run repository, architecture, style, release, and documentation contracts;
11. replay the mailbox from the exact advertised base and compare Git trees.

State identity domains and publication evidence schemas are durable core protocols. Concrete storage protocols belong to their provider repositories. Treat their changes separately from the C++ ABI. A repository release may preserve all three; a C++ ABI change need not imply a storage migration; a storage change always requires explicit migration design.

Any appearance of `libpkgsource`, `libpkgbuild`, `libpkgimage`, `libpkgplan`, `libpkgapply`, or `libpkgstate-*` in core build metadata, installed headers, pkg-config output, or dynamic linkage is a release blocker. Any concrete storage provider, filesystem syscall, or provider tool in the core is also a release blocker.
