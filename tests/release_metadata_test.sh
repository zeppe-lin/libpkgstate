#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
fail() { echo "release-metadata-test: $*" >&2; exit 1; }
grep -F "version: '2.0.0'" "$root/meson.build" >/dev/null || fail 'Meson version is not 2.0.0'
grep -F 'PROJECT_NUMBER         = 2.0.0' "$root/Doxyfile" >/dev/null || fail 'Doxygen version is not 2.0.0'
for input in include/libpkgstate-source include/libpkgstate-build include/libpkgstate-plan include/libpkgstate-apply; do
  grep -F "$input" "$root/Doxyfile" >/dev/null || fail "Doxygen input is missing: $input"
done
grep -F "soversion: '3'" "$root/src/meson.build" >/dev/null || fail 'core soversion is not 3'
grep -F "soversion: '1'" "$root/source_adapter/meson.build" >/dev/null || fail 'source adapter soversion is not 1'
grep -F "soversion: '1'" "$root/build_adapter/meson.build" >/dev/null || fail 'build adapter soversion is not 1'
grep -F "soversion: '2'" "$root/adapter/meson.build" >/dev/null || fail 'planner adapter soversion is not 2'
grep -F "soversion: '2'" "$root/apply_adapter/meson.build" >/dev/null || fail 'apply adapter soversion is not 2'
grep -F "version: '>=1.0.0'" "$root/meson.build" >/dev/null || fail 'libpkgsource/libpkgbuild floor is missing'
grep -F "version: '>=0.3.0'" "$root/meson.build" >/dev/null || fail 'libpkgimage floor is missing'
grep -F "version: '>=0.2.0'" "$root/meson.build" >/dev/null || fail 'libpkgplan floor is missing'
grep -F "version: '>=0.1.0'" "$root/meson.build" >/dev/null || fail 'libpkgapply floor is missing'
grep -F "get_option('build_adapter').enabled()" "$root/meson.build" >/dev/null || fail 'build adapter dependency scope is missing'
if grep -E 'libpkg(source|build|image|plan|apply)' "$root/src/meson.build" >/dev/null; then
  fail 'core metadata references an optional authority library'
fi
grep -F 'requires_private: [libcrypto_dep]' "$root/src/meson.build" >/dev/null || fail 'core private crypto closure is missing'
