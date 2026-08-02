#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
fail() { echo "release-metadata-test: $*" >&2; exit 1; }
grep -F "version: '2.5.1'" "$root/meson.build" >/dev/null || fail 'Meson version is not 2.5.0'
grep -F 'PROJECT_NUMBER         = 2.5.1' "$root/Doxyfile" >/dev/null || fail 'Doxygen version is not 2.5.0'
for input in include/libpkgstate-source include/libpkgstate-build include/libpkgstate-plan include/libpkgstate-apply; do
  grep -F "$input" "$root/Doxyfile" >/dev/null || fail "Doxygen input is missing: $input"
done
grep -F "soversion: '3'" "$root/src/meson.build" >/dev/null || fail 'core soversion is not 3'
grep -F "soversion: '1'" "$root/source_adapter/meson.build" >/dev/null || fail 'source adapter soversion is not 1'
grep -F "soversion: '1'" "$root/build_adapter/meson.build" >/dev/null || fail 'build adapter soversion is not 1'
grep -F "soversion: '2'" "$root/adapter/meson.build" >/dev/null || fail 'planner adapter soversion is not 2'
grep -F "soversion: '3'" "$root/apply_adapter/meson.build" >/dev/null || fail 'apply adapter soversion is not 3'
grep -F "version: '>=2.1.0'" "$root/meson.build" >/dev/null || fail 'libpkgapply 2.1 authority floor is missing'
grep -F "version: '>=0.3.0'" "$root/meson.build" >/dev/null || fail 'libpkgimage floor is missing'
grep -F "version: '>=0.2.0'" "$root/meson.build" >/dev/null || fail 'libpkgplan floor is missing'
grep -F "'libpkgstate-source >= 2.3.0'" "$root/build_adapter/meson.build" >/dev/null || fail 'rebuilt source-adapter floor is missing'
grep -F "'libpkgstate-build >= 2.3.0'" "$root/apply_adapter/meson.build" >/dev/null || fail 'rebuilt build-adapter floor is missing'
grep -F "get_option('build_adapter').enabled()" "$root/meson.build" >/dev/null || fail 'build adapter dependency scope is missing'
if grep -E 'libpkg(source|build|image|plan|apply)' "$root/src/meson.build" >/dev/null; then
  fail 'core metadata references an optional authority library'
fi
grep -F 'requires_private: [libcrypto_dep]' "$root/src/meson.build" >/dev/null || fail 'core private crypto closure is missing'
grep -F '2.5.1' "$root/HISTORY.md" >/dev/null || fail 'history omits 2.5.1'
grep -F 'libpkgstate-generation-v3' "$root/HISTORY.md" >/dev/null || fail 'history omits retained storage generation'
grep -F 'core `libpkgstate` remains at soversion 3' "$root/HISTORY.md" >/dev/null || fail 'history omits stable core ABI'
grep -F 'retaining adapter soversion 3 and core soversion 3' "$root/HISTORY.md" >/dev/null || fail 'history omits stable apply and core ABI'
grep -F 'generation-v3 installed-state storage does not change' "$root/HISTORY.md" >/dev/null || fail 'history omits storage compatibility statement'

grep -F "'libpkgapply >= 2.1.0'" "$root/apply_adapter/meson.build" >/dev/null || fail 'application adapter libpkgapply floor is not 2.1.0'

grep -F "'publication_codec.cpp'" "$root/src/meson.build" >/dev/null || fail 'core source omits publication codec'
grep -F "'../include/libpkgstate/publication_codec.h'" "$root/src/meson.build" >/dev/null || fail 'core install omits publication codec header'
grep -F 'state_publication_request_encoding_version = 2' "$root/include/libpkgstate/publication_codec.h" >/dev/null || fail 'request codec version is not 2'
grep -F 'state_publication_receipt_encoding_version = 2' "$root/include/libpkgstate/publication_codec.h" >/dev/null || fail 'receipt codec version is not 2'

grep -F 'ZLSPRQST' "$root/HISTORY.md" >/dev/null || fail 'history omits request house framing'
grep -F 'ZLSPRCPT' "$root/HISTORY.md" >/dev/null || fail 'history omits receipt house framing'
grep -F 'version-1 decoder' "$root/HISTORY.md" >/dev/null || fail 'history omits narrow v1 compatibility'
