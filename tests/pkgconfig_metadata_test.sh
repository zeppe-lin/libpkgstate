#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu
pc=$1

fail()
{
  echo "pkgconfig-metadata-test: $*" >&2
  exit 1
}

test -s "$pc" || fail "missing generated metadata: $pc"

test "$(grep -c '^Requires:' "$pc")" -eq 1 ||
  fail 'generated metadata does not contain exactly one Requires field'

grep -E \
  '^Requires:[[:space:]]*libpkgimage[[:space:]]*>=[[:space:]]*0\.3\.0[[:space:]]*$' \
  "$pc" >/dev/null ||
  fail 'generated metadata does not require libpkgimage 0.3.0 exactly once'

if grep -E '^Requires\.private:.*libpkgimage' "$pc" >/dev/null; then
  fail 'public libpkgimage dependency was demoted to Requires.private'
fi
