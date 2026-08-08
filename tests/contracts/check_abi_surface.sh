#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

if [ "$#" -ne 3 ]; then
  echo "usage: $0 LIBRARY MANIFEST NM" >&2
  exit 2
fi

library=$1
manifest=$2
nm=$3

base=${TMPDIR:-/tmp}/libpkgstate-abi.$$
actual=$base.actual
reviewed=$base.reviewed
trap 'rm -f "$actual" "$reviewed"' EXIT HUP INT TERM

# The manifest is a byte-ordered protocol artifact. Do not let the caller's
# locale change symbol ordering and manufacture an ABI difference.
LC_ALL=C
export LC_ALL

sort -u "$manifest" >"$reviewed"
if ! cmp -s "$manifest" "$reviewed"; then
  echo "reviewed ABI manifest is not uniquely C-locale sorted: $manifest" >&2
  diff -u "$manifest" "$reviewed" >&2 || true
  exit 1
fi

# Every reviewed C++ export must carry the pkgstate namespace in its mangled
# name. Pure libstdc++ implementation instantiations are compiler artifacts,
# not state authority, and must remain local to the shared object.
if grep -v '8pkgstate' "$manifest" >/dev/null; then
  echo "reviewed ABI manifest contains a foreign implementation export" >&2
  grep -v '8pkgstate' "$manifest" >&2 || true
  exit 1
fi

"$nm" -D --defined-only "$library" |
  awk '{print $3}' |
  sed '/^$/d' |
  sort -u >"$actual"

if ! cmp -s "$manifest" "$actual"; then
  echo "exported ABI differs from reviewed manifest: $manifest" >&2
  diff -u "$manifest" "$actual" >&2 || true
  exit 1
fi
