#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu

metadata=$1

fail()
{
  echo "plan-adapter-metadata-test: $*" >&2
  exit 1
}

grep -F 'Name: libpkgstate-plan' "$metadata" >/dev/null ||
  fail 'pkg-config metadata has the wrong module name'
grep -F 'libpkgstate = 0.3.0' "$metadata" >/dev/null ||
  fail 'pkg-config metadata omits the exact libpkgstate dependency'
grep -F 'libpkgplan >= 0.1.0' "$metadata" >/dev/null ||
  fail 'pkg-config metadata omits the libpkgplan dependency floor'
grep -F -- '-lpkgstate-plan' "$metadata" >/dev/null ||
  fail 'pkg-config metadata omits the adapter library'
