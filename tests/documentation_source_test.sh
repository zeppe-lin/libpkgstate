#!/bin/sh
set -eu
root=$1
for f in README.md DESIGN.md STORAGE.md TESTING.md MIGRATION.md docs/architecture.md docs/integration.md docs/testing.md docs/code-style.md docs/abi.md docs/meson.build docs/history/3.0-adapter-extraction.md man/libpkgstate.3.scdoc man/pkgstate-generation.5.scdoc man/pkgstate-check.1.scdoc; do test -s "$root/$f" || { echo "missing documentation: $f" >&2; exit 1; }; done
grep -F 'libpkgstate-source' "$root/README.md" >/dev/null
grep -F 'independent repositories' "$root/README.md" >/dev/null
for stale in include/libpkgstate-source include/libpkgstate-build include/libpkgstate-plan include/libpkgstate-apply; do
  if grep -F "$stale" "$root/Doxyfile" >/dev/null; then echo "stale Doxygen input: $stale" >&2; exit 1; fi
done
