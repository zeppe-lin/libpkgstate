#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
fail() { echo "publication-codec-source-test: $*" >&2; exit 1; }
header=$root/include/libpkgstate/publication_codec.h
source=$root/src/publication_codec.cpp
for file in "$header" "$source" "$root/tests/publication_codec_test.cpp"; do
  test -s "$file" || fail "missing ${file#"$root"/}"
done
for contract in \
  'encode_state_publication_request' \
  'decode_state_publication_request' \
  'encode_state_publication_receipt' \
  'decode_state_publication_receipt' \
  'expected_snapshot_mismatch' \
  'actual_prior_mismatch' \
  'checksum_mismatch'
do
  grep -F "$contract" "$header" "$source" >/dev/null ||
    fail "codec omits $contract"
done
for forbidden in \
  'canonical_generation_store' \
  'compare_and_publish(' \
  'begin_publication(' \
  'std::filesystem' \
  'open(' \
  'rename(' \
  'unlink('
do
  if grep -F "$forbidden" "$source" >/dev/null; then
    fail "codec imports effectful boundary: $forbidden"
  fi
done
grep -F "'publication_codec.cpp'" "$root/src/meson.build" >/dev/null ||
  fail 'core Meson omits publication_codec.cpp'
grep -F "'../include/libpkgstate/publication_codec.h'" "$root/src/meson.build" >/dev/null ||
  fail 'core install omits publication_codec.h'
grep -F "['publication_codec', 'reopen durable publication evidence']" \
  "$root/tests/meson.build" >/dev/null ||
  fail 'Meson omits publication codec runtime test'
grep -F "['publication-codec', 'libpkgstate/publication_codec.h']" \
  "$root/tests/meson.build" >/dev/null ||
  fail 'Meson omits publication codec public-header test'
