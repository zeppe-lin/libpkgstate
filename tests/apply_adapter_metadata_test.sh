#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu

build_root=$1
metadata=$build_root/meson-private/libpkgstate-apply.pc

fail()
{
  echo "apply-adapter-metadata-test: $*" >&2
  if test -n "${metadata:-}" && test -f "$metadata"; then
    echo '--- generated metadata ---' >&2
    cat "$metadata" >&2
    echo '--- end generated metadata ---' >&2
  fi
  exit 1
}

if test ! -s "$metadata"; then
  metadata=$(find "$build_root" -type f -name libpkgstate-apply.pc -print |
    sed -n '1p')
fi

test -n "${metadata:-}" && test -s "$metadata" ||
  fail 'generated libpkgstate-apply.pc was not found'

name=$(sed -n 's/^Name:[[:space:]]*//p' "$metadata")
test "$name" = libpkgstate-apply ||
  fail "pkg-config module name is '$name', expected 'libpkgstate-apply'"

requires=$(sed -n 's/^Requires:[[:space:]]*//p' "$metadata" | tr '\n' ',')
printf '%s\n' "$requires" |
  grep -Eq '(^|,)[[:space:]]*libpkgstate[[:space:]]*>=[[:space:]]*1\.0\.0([[:space:]]*,|$)' ||
  fail 'pkg-config metadata omits the libpkgstate 1.0.0 dependency floor'
printf '%s\n' "$requires" |
  grep -Eq '(^|,)[[:space:]]*libpkgapply[[:space:]]*>=[[:space:]]*0\.1\.0([[:space:]]*,|$)' ||
  fail 'pkg-config metadata omits the libpkgapply dependency floor'

libs=$(sed -n 's/^Libs:[[:space:]]*//p' "$metadata")
printf ' %s \n' "$libs" |
  grep -F ' -lpkgstate-apply ' >/dev/null ||
  fail 'pkg-config metadata omits the adapter library'
