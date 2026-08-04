#!/bin/sh
set -eu
root=$1
for f in README.md DESIGN.md TESTING.md MIGRATION.md docs/architecture.md docs/integration.md docs/testing.md docs/code-style.md docs/abi.md docs/meson.build docs/history/3.0-adapter-extraction.md man/libpkgstate.3.scdoc man/pkgstate_generation_codec.3.scdoc; do test -s "$root/$f" || { echo "missing documentation: $f" >&2; exit 1; }; done
grep -F 'libpkgstate-source' "$root/README.md" >/dev/null
grep -F 'independent repositories' "$root/README.md" >/dev/null
for stale in include/libpkgstate-source include/libpkgstate-build include/libpkgstate-plan include/libpkgstate-apply; do
  if grep -F "$stale" "$root/Doxyfile" >/dev/null; then echo "stale Doxygen input: $stale" >&2; exit 1; fi
done

for stale in STORAGE.md include/libpkgstate/canonical_generation_store.h man/pkgstate-generation.5.scdoc man/pkgstate-check.1.scdoc; do test ! -e "$root/$stale" || { echo "provider documentation retained: $stale" >&2; exit 1; }; done

grep -F 'canonical generation-v3' "$root/docs/architecture.md" >/dev/null || { echo "state-owned generation protocol undocumented" >&2; exit 1; }
python3 "$root/tools/check-public-documentation.py" \
  "$root" libpkgstate libpkgstate.h

python3 "$root/tools/check-man-markdown.py" \
  --root "$root" --project libpkgstate --version 3.0.0
python3 "$root/tools/check-html-manifest.py" \
  --root "$root" --project libpkgstate
