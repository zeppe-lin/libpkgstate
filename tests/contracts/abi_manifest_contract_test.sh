#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

root=$1
fail(){ echo "abi-manifest-contract: $*" >&2; exit 1; }

tmp=${TMPDIR:-/tmp}/libpkgstate-abi-manifest.$$
valid=$tmp.valid
foreign=$tmp.foreign
unsorted=$tmp.unsorted
map=$tmp.map
trap 'rm -f "$valid" "$foreign" "$unsorted" "$map"' EXIT HUP INT TERM

manifest=$root/abi/libpkgstate.exports
generator=$root/tools/generate-elf-export-script.sh

LC_ALL=C
export LC_ALL

"$generator" "$manifest" >"$map" || fail 'reviewed manifest was rejected'
grep -F '  local:' "$map" >/dev/null || fail 'generated map omits local boundary'
grep -F '    *;' "$map" >/dev/null || fail 'generated map omits wildcard localization'

cp "$manifest" "$valid"
printf '%s\n' '_ZNSt6vectorIhSaIhEE17_M_realloc_appendIJhEEEvDpOT_' >>"$foreign"
cat "$valid" >>"$foreign"
if "$generator" "$foreign" >/dev/null 2>&1; then
  fail 'foreign libstdc++ export was accepted'
fi

{
  sed -n '2p' "$valid"
  sed -n '1p' "$valid"
  sed -n '3,$p' "$valid"
} >"$unsorted"
if "$generator" "$unsorted" >/dev/null 2>&1; then
  fail 'non-canonical manifest order was accepted'
fi
