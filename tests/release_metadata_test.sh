#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
fail() { echo "release-metadata-test: $*" >&2; exit 1; }
grep -F "version: '1.0.0'" "$root/meson.build" >/dev/null || fail 'Meson version is not 1.0.0'
grep -F 'PROJECT_NUMBER         = 1.0.0' "$root/Doxyfile" >/dev/null || fail 'Doxygen version is not 1.0.0'
grep -F 'include/libpkgstate-source' "$root/Doxyfile" >/dev/null || fail 'Doxygen source adapter input is missing'
grep -F "soversion: '2'" "$root/src/meson.build" >/dev/null || fail 'core soversion is not 2'
grep -F "soversion: '1'" "$root/source_adapter/meson.build" >/dev/null || fail 'source adapter soversion is not 1'
grep -F "soversion: '2'" "$root/adapter/meson.build" >/dev/null || fail 'planner adapter soversion is not 2'
grep -F "soversion: '1'" "$root/apply_adapter/meson.build" >/dev/null || fail 'apply adapter soversion is not 1'
grep -F "version: '>=1.0.0'" "$root/meson.build" >/dev/null || fail 'libpkgsource floor is missing'
grep -F "required: get_option('source_adapter')" "$root/meson.build" >/dev/null || fail 'source adapter dependency scope is wrong'
grep -F "version: '>=0.2.0'" "$root/meson.build" >/dev/null || fail 'libpkgplan floor is missing'
grep -F "version: '>=0.1.0'" "$root/meson.build" >/dev/null || fail 'libpkgapply floor is missing'
if grep -E 'libpkg(source|image|plan|apply)' "$root/src/meson.build" >/dev/null; then
  fail 'core metadata references an optional authority library'
fi
grep -F 'requires_private: [libcrypto_dep]' "$root/src/meson.build" >/dev/null || fail 'core private crypto closure is missing'
grep -F '1.0.0' "$root/HISTORY.md" >/dev/null || fail 'history omits 1.0.0'
grep -F 'libpkgstate-generation-v2' "$root/HISTORY.md" >/dev/null || fail 'history omits storage reset'
grep -F 'core `libpkgstate` advances to soversion 2' "$root/HISTORY.md" >/dev/null || fail 'history omits core ABI'
grep -F '`libpkgstate-plan` advances to soversion 2' "$root/HISTORY.md" >/dev/null || fail 'history omits planner ABI'
grep -F '`libpkgstate-apply` advances to soversion 1' "$root/HISTORY.md" >/dev/null || fail 'history omits apply ABI'
