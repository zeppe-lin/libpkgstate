#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu

build_root=$1
metadata=$build_root/meson-private/libpkgstate-build.pc

fail()
{
  echo "build-adapter-metadata-test: $*" >&2
  if test -n "${metadata:-}" && test -f "$metadata"; then
    echo '--- generated metadata ---' >&2
    cat "$metadata" >&2
    echo '--- end generated metadata ---' >&2
  fi
  exit 1
}

if test ! -s "$metadata"; then
  metadata=$(find "$build_root" -type f -name libpkgstate-build.pc -print |
    sed -n '1p')
fi

test -n "${metadata:-}" && test -s "$metadata" ||
  fail 'generated libpkgstate-build.pc was not found'

name=$(sed -n 's/^Name:[[:space:]]*//p' "$metadata")
test "$name" = libpkgstate-build ||
  fail "pkg-config module name is '$name', expected 'libpkgstate-build'"

# Meson de-duplicates same-build requirements. libpkgstate-source carries the
# libpkgstate >= 2.0.0 requirement, so the build adapter metadata need not
# repeat the core module literally.
requires=$(sed -n 's/^Requires:[[:space:]]*//p' "$metadata" | tr '\n' ',')
for dependency in \
  'libpkgstate-source[[:space:]]*>=[[:space:]]*2\.0\.0' \
  'libpkgbuild[[:space:]]*>=[[:space:]]*1\.0\.0' \
  'libpkgimage[[:space:]]*>=[[:space:]]*0\.3\.0'
do
  printf '%s\n' "$requires" |
    grep -Eq "(^|,)[[:space:]]*$dependency([[:space:]]*,|$)" ||
    fail "pkg-config metadata omits $dependency"
done

libs=$(sed -n 's/^Libs:[[:space:]]*//p' "$metadata")
printf ' %s \n' "$libs" |
  grep -F ' -lpkgstate-build ' >/dev/null ||
  fail 'pkg-config metadata omits the adapter library'
