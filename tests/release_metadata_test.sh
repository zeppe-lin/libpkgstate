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

grep -F "version: '0.6.0'" "$root/meson.build" >/dev/null ||
  fail 'Meson project version is not 0.6.0'

grep -F 'PROJECT_NUMBER         = 0.6.0' "$root/Doxyfile" >/dev/null ||
  fail 'Doxygen project version is not 0.6.0'

grep -F "soversion: '1'" "$root/src/meson.build" >/dev/null ||
  fail 'core libpkgstate soversion is not 1'

grep -F "soversion: '1'" "$root/adapter/meson.build" >/dev/null ||
  fail 'planner adapter soversion is not 1'

grep -F "soversion: '0'" "$root/apply_adapter/meson.build" >/dev/null ||
  fail 'application adapter soversion is not 0'

grep -F "'libcrypto'" "$root/meson.build" >/dev/null ||
  fail 'Meson metadata omits the canonical digest backend'

grep -F 'requires_private: [libcrypto_dep]' \
  "$root/src/meson.build" >/dev/null ||
  fail 'pkg-config metadata omits private libcrypto closure'

grep -F "version: '>=0.3.0'" "$root/meson.build" >/dev/null ||
  fail 'Meson dependency floor does not require libpkgimage 0.3.0'

grep -F "required: get_option('tools')" "$root/meson.build" >/dev/null ||
  fail 'libpkgimage is not scoped to the optional tools feature'

grep -F "version: '>=0.2.0'" "$root/meson.build" >/dev/null ||
  fail 'Meson dependency floor does not require libpkgplan 0.2.0'

grep -F "required: get_option('planner_adapter')" \
  "$root/meson.build" >/dev/null ||
  fail 'libpkgplan is not scoped to the optional adapter feature'

grep -F "version: '>=0.1.0'" "$root/meson.build" >/dev/null ||
  fail 'Meson dependency floor does not require libpkgapply 0.1.0'

grep -F "required: get_option('application_adapter')" \
  "$root/meson.build" >/dev/null ||
  fail 'libpkgapply is not scoped to the optional application adapter'

if grep -E 'libpkg(image|plan|apply)' "$root/src/meson.build" >/dev/null; then
  fail 'core library metadata references an optional composition dependency'
fi

grep -F 'dependencies: [libpkgstate_dep, libpkgimage_dep]' \
  "$root/tools/meson.build" >/dev/null ||
  fail 'pkginfo frontend metadata omits libpkgimage'

grep -F "'libpkgstate = ' + meson.project_version()" \
  "$root/adapter/meson.build" >/dev/null ||
  fail 'planner adapter metadata omits exact core version'

grep -F "'libpkgstate = ' + meson.project_version()" \
  "$root/apply_adapter/meson.build" >/dev/null ||
  fail 'application adapter metadata omits exact core version'

grep -F "'libpkgapply >= 0.1.0'" \
  "$root/apply_adapter/meson.build" >/dev/null ||
  fail 'application adapter metadata omits libpkgapply floor'

grep -F '0.6.0' "$root/HISTORY.md" >/dev/null ||
  fail 'history omits release 0.6.0'

grep -F 'application adapter begins at soversion 0' \
  "$root/HISTORY.md" >/dev/null ||
  fail 'history omits the application adapter ABI decision'

grep -F 'core soversion advances from 0 to 1' \
  "$root/HISTORY.md" >/dev/null ||
  fail 'history omits the core ABI break'

grep -F 'adapter soversion advances from 0 to 1' \
  "$root/HISTORY.md" >/dev/null ||
  fail 'history omits the adapter ABI decision'
