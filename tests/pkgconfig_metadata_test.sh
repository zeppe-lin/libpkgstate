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

if grep -E '^Requires(\.private)?:.*libpkgimage' "$pc" >/dev/null; then
  fail 'core pkg-config metadata still exposes libpkgimage'
fi

if grep -E '^Libs(\.private)?:.*-lpkgimage' "$pc" >/dev/null; then
  fail 'core pkg-config metadata still links libpkgimage'
fi
