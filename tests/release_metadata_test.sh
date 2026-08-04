#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
fail(){ echo "release-metadata-test: $*" >&2; exit 1; }
grep -F "version: '3.0.0'" "$root/meson.build" >/dev/null || fail 'Meson version is not 3.0.0'
grep -F 'PROJECT_NUMBER         = 3.0.0' "$root/Doxyfile" >/dev/null || fail 'Doxygen version is not 3.0.0'
grep -F 'INPUT                  = include/libpkgstate' "$root/Doxyfile" >/dev/null || fail 'Doxygen input is not core-only'
grep -F "soversion: '3'" "$root/src/meson.build" >/dev/null || fail 'core soversion is not 3'
grep -F 'requires_private: [libcrypto_dep]' "$root/src/meson.build" >/dev/null || fail 'private crypto closure is missing'
grep -F '3.0.0' "$root/HISTORY.md" >/dev/null || fail 'history omits 3.0.0'
grep -F 'libpkgstate-posix' "$root/HISTORY.md" >/dev/null || fail 'history omits provider extraction'
grep -F "'publication_codec.cpp'" "$root/src/meson.build" >/dev/null || fail 'core source omits publication codec'
! grep -F "'canonical_generation_store.cpp'" "$root/src/meson.build" >/dev/null || fail 'core retains generation backend'
grep -F 'state_publication_request_encoding_version = 2' "$root/include/libpkgstate/publication_codec.h" >/dev/null || fail 'request codec version is not 2'
grep -F 'state_publication_receipt_encoding_version = 2' "$root/include/libpkgstate/publication_codec.h" >/dev/null || fail 'receipt codec version is not 2'
test -s "$root/abi/libpkgstate.exports" || fail 'reviewed core ABI manifest is absent'
grep -F "'-DPKGSTATE_BUILDING_LIBRARY'" "$root/src/meson.build" >/dev/null || fail 'core export build contract is absent'
