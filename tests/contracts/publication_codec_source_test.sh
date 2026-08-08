#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
fail() { echo "publication-codec-source-test: $*" >&2; exit 1; }
header=$root/include/libpkgstate/publication_codec.h
source=$root/src/publication_codec.cpp
for file in "$header" "$source" "$root/tests/protocol/publication_request_codec_test.cpp" "$root/tests/protocol/publication_receipt_codec_test.cpp"; do
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
grep -F 'state_publication_request_encoding_version = 1' "$header" >/dev/null ||
  fail 'request codec version is not 1'
grep -F 'state_publication_receipt_encoding_version = 1' "$header" >/dev/null ||
  fail 'receipt codec version is not 1'
grep -F "'Z', 'L', 'S', 'P', 'R', 'Q', 'S', 'T'" "$source" >/dev/null ||
  fail 'request codec omits ZLSPRQST framing'
grep -F "'Z', 'L', 'S', 'P', 'R', 'C', 'P', 'T'" "$source" >/dev/null ||
  fail 'receipt codec omits ZLSPRCPT framing'
for retired in legacy_request_magic legacy_receipt_magic legacy_v1 house_v2; do
  if grep -F "$retired" "$source" >/dev/null; then
    fail "codec retains experimental compatibility path: $retired"
  fi
done
grep -F 'output.raw(request_magic)' "$source" >/dev/null ||
  fail 'request encoder does not emit current framing'
grep -F 'output.raw(receipt_magic)' "$source" >/dev/null ||
  fail 'receipt encoder does not emit current framing'
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
grep -F "['publication-request-codec', 'reopen publication requests']" \
  "$root/tests/meson.build" >/dev/null ||
  fail 'Meson omits publication request codec runtime test'
grep -F "['publication-receipt-codec', 'reopen publication receipts']" \
  "$root/tests/meson.build" >/dev/null ||
  fail 'Meson omits publication receipt codec runtime test'
grep -F "['publication-codec', 'libpkgstate/publication_codec.h']" \
  "$root/tests/meson.build" >/dev/null ||
  fail 'Meson omits publication codec public-header test'
