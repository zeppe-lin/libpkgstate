#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
fail() { echo "test-layout-contract: $*" >&2; exit 1; }

for dir in contracts fixtures header integration protocol support unit; do
  test -d "$root/tests/$dir" || fail "missing tests/$dir"
done

if find "$root/tests" -maxdepth 1 -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.sh' \) | grep . >/dev/null; then
  fail 'test implementation remains in tests/ root'
fi

for suite in unit integration protocol header contract; do
  grep -F "suite: '$suite'" "$root/tests/meson.build" >/dev/null ||
    fail "Meson omits $suite suite"
done

test -s "$root/tests/fixtures/state.h" || fail 'state fixture is absent'
test -s "$root/tests/support/test.h" || fail 'test support is absent'
test -s "$root/tests/support/publication_codec.h" || fail 'publication codec support is absent'
test -s "$root/tests/integration/installed_package_test.cpp" || fail 'installed-package integration test is absent'
test -s "$root/tests/protocol/publication_request_codec_test.cpp" || fail 'request codec protocol test is absent'
test -s "$root/tests/protocol/publication_receipt_codec_test.cpp" || fail 'receipt codec protocol test is absent'
! test -e "$root/tests/protocol/publication_codec_test.cpp" || fail 'publication codec omnibus test remains'

echo 'test-layout-contract: ok'
