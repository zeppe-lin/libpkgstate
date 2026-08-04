#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
fail(){ echo "architecture-contract: $*" >&2; exit 1; }
for path in source_adapter build_adapter adapter apply_adapter include/libpkgstate-source include/libpkgstate-build include/libpkgstate-plan include/libpkgstate-apply include/libpkgstate/canonical_generation_store.h src/canonical_generation_store.cpp src/generation_codec.cpp tools/pkgstate_check.cpp; do [ ! -e "$root/$path" ] || fail "foreign adapter retained: $path"; done
for token in libpkgsource libpkgbuild libpkgimage libpkgplan libpkgapply libpkgstate-source libpkgstate-build libpkgstate-plan libpkgstate-apply source_adapter build_adapter planner_adapter application_adapter; do
  if grep -R -F "$token" "$root/meson.build" "$root/meson.options" "$root/src" "$root/include/libpkgstate" 2>/dev/null | grep -v architecture_contract_test >/dev/null; then fail "core build or API mentions $token"; fi
done
grep -F 'Concrete storage, lock, selector, durability, and diagnostic mechanisms are provider authority' "$root/docs/architecture.md" >/dev/null || fail 'provider placement is undocumented'
grep -F "gnu_symbol_visibility: 'hidden'" "$root/src/meson.build" >/dev/null || fail 'hidden core visibility is not enforced'
if grep -R -E '#include <(fcntl|unistd|sys/|linux/)|\b(openat|renameat|unlinkat|mkdirat|flock|fsync)\b' "$root/src" "$root/include/libpkgstate" >/dev/null; then fail 'core retains host storage mechanism'; fi
test -s "$root/abi/libpkgstate.exports" || fail 'reviewed core export manifest is absent'
grep -F "libpkgstate/export.h" "$root/src/meson.build" >/dev/null || fail 'export contract is not installed'
