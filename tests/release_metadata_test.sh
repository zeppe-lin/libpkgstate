#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
fail(){ echo "release-metadata-test: $*" >&2; exit 1; }
grep -F "version: '3.0.0'" "$root/meson.build" >/dev/null || fail 'Meson version is not 3.0.0'
grep -F 'PROJECT_NUMBER         = 3.0.0' "$root/Doxyfile" >/dev/null || fail 'Doxygen version is not 3.0.0'
grep -F 'INPUT                  = include/libpkgstate' "$root/Doxyfile" >/dev/null || fail 'Doxygen input is not core-only'
grep -F "soversion: '4'" "$root/src/meson.build" >/dev/null || fail 'core soversion is not 4'
grep -F 'requires_private: [libcrypto_dep]' "$root/src/meson.build" >/dev/null || fail 'private crypto closure is missing'
grep -F '3.0.0' "$root/HISTORY.md" >/dev/null || fail 'history omits 3.0.0'
grep -F 'libpkgstate-posix' "$root/HISTORY.md" >/dev/null || fail 'history omits provider extraction'
grep -F "'publication_codec.cpp'" "$root/src/meson.build" >/dev/null || fail 'core source omits publication codec'
! grep -F "'canonical_generation_store.cpp'" "$root/src/meson.build" >/dev/null || fail 'core retains generation backend'
grep -F 'state_publication_request_encoding_version = 1' "$root/include/libpkgstate/publication_codec.h" >/dev/null || fail 'request codec version is not 1'
grep -F 'state_publication_receipt_encoding_version = 1' "$root/include/libpkgstate/publication_codec.h" >/dev/null || fail 'receipt codec version is not 1'
test -s "$root/abi/libpkgstate.exports" || fail 'reviewed core ABI manifest is absent'
grep -F "'-DPKGSTATE_BUILDING_LIBRARY'" "$root/src/meson.build" >/dev/null || fail 'core export build contract is absent'

grep -F "'generation_codec.cpp'" "$root/src/meson.build" >/dev/null || fail 'core source omits generation codec'
grep -F "'../include/libpkgstate/generation_codec.h'" "$root/src/meson.build" >/dev/null || fail 'core install omits generation codec'
grep -F 'canonical_generation_storage_version = 1' "$root/include/libpkgstate/generation_codec.h" >/dev/null || fail 'generation codec version is not 1'
grep -F 'libpkgstate-generation-v1' "$root/include/libpkgstate/generation_codec.h" >/dev/null || fail 'generation codec format is not v1'
grep -F 'pkgstate/package-source-record/1' "$root/include/libpkgstate/digest.h" >/dev/null || fail 'source-record identity protocol is not version 1'
if grep -R -F 'source_recipe_identity' "$root/include" "$root/src" >/dev/null; then fail 'retired source recipe identity remains in core'; fi
! grep -v '8pkgstate' "$root/abi/libpkgstate.exports" >/dev/null || fail 'core ABI manifest contains foreign implementation exports'
