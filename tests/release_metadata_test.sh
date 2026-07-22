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

grep -F "required: get_option('tools')" "$root/meson.build" >/dev/null ||
  fail 'libpkgimage is not scoped to the optional tools feature'

if grep -F 'libpkgimage' "$root/src/meson.build" >/dev/null; then
  fail 'core library metadata still references libpkgimage'
fi

grep -F 'dependencies: [libpkgstate_dep, libpkgimage_dep]' \
  "$root/tools/meson.build" >/dev/null ||
  fail 'pkginfo frontend metadata omits libpkgimage'

grep -F '0.3.0' "$root/HISTORY.md" >/dev/null ||
  fail 'history omits release 0.3.0'

grep -F 'public libpkgstate ABI and soversion remain unchanged' \
  "$root/HISTORY.md" >/dev/null ||
  fail 'history omits the ABI decision'
