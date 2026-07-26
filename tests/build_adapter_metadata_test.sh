#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
build=$1
pc=$build/meson-private/libpkgstate-build.pc
fail() { echo "build-adapter-metadata-test: $*" >&2; exit 1; }
test -s "$pc" || fail "missing $pc"
for dependency in \
  'libpkgstate >= 2.0.0' \
  'libpkgstate-source >= 2.0.0' \
  'libpkgbuild >= 1.0.0' \
  'libpkgimage >= 0.3.0'
do
  grep -F "$dependency" "$pc" >/dev/null ||
    fail "metadata omits $dependency"
done
