# Maintaining libpkgstate

Before release, qualify every commit boundary, build shared and static configurations separately with GCC and Clang, run ASan and UBSan, compile every public header independently, inspect pkg-config and dynamic dependency closure, lint generated manuals, run strict Doxygen, stage-install and compile a consumer, and replay the advertised mailbox from the exact base.

The core dependency graph is closed over `libcrypto`. Any appearance of `libpkgsource`, `libpkgbuild`, `libpkgimage`, `libpkgplan`, `libpkgapply`, or `libpkgstate-*` in core build metadata is a release blocker.

Generation-v3 records and identity domains are durable protocols. Changing them requires explicit migration design; the owner never silently interprets an old generation as a new one.
