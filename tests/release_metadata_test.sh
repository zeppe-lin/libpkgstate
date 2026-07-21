#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu
root=$1

fail()
{
  echo "release-metadata-test: $*" >&2
  exit 1
}

grep -F "version: '0.3.0'" "$root/meson.build" >/dev/null ||
  fail 'Meson project version is not 0.3.0'

grep -F 'PROJECT_NUMBER         = 0.3.0' "$root/Doxyfile" >/dev/null ||
  fail 'Doxygen project version is not 0.3.0'

grep -F "soversion: '0'" "$root/src/meson.build" >/dev/null ||
  fail 'libpkgstate soversion changed unexpectedly'

grep -F "version: '>=0.3.0'" "$root/meson.build" >/dev/null ||
  fail 'Meson dependency floor does not require libpkgimage 0.3.0'

grep -F 'requires: [libpkgimage_dep]' \
  "$root/src/meson.build" >/dev/null ||
  fail 'pkg-config metadata does not publish the libpkgimage dependency'

grep -F '0.3.0' "$root/HISTORY.md" >/dev/null ||
  fail 'history omits release 0.3.0'

grep -F 'public libpkgstate ABI and soversion remain unchanged' \
  "$root/HISTORY.md" >/dev/null ||
  fail 'history omits the ABI decision'
