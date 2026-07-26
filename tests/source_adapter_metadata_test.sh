#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
build_root=$1
metadata=$build_root/meson-private/libpkgstate-source.pc
[ -s "$metadata" ] || metadata=$(find "$build_root" -type f -name libpkgstate-source.pc -print | sed -n '1p')
[ -n "${metadata:-}" ] && [ -s "$metadata" ] || {
  echo 'source-adapter-metadata-test: generated metadata not found' >&2
  exit 1
}
grep -F 'Name: libpkgstate-source' "$metadata" >/dev/null
grep -E 'Requires:.*libpkgstate[[:space:]]*>=[[:space:]]*1\.0\.0' "$metadata" >/dev/null
grep -E 'Requires:.*libpkgsource[[:space:]]*>=[[:space:]]*1\.0\.0' "$metadata" >/dev/null
grep -E 'Libs:.*-lpkgstate-source' "$metadata" >/dev/null
