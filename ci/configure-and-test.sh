#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

usage()
{
  echo "usage: $0 BUILD-DIR {shared|static} [MESON-ARG ...]" >&2
  exit 2
}

[ "$#" -ge 2 ] || usage
build_dir=$1
link_mode=$2
shift 2

case $link_mode in
  shared|static) ;;
  *) usage ;;
esac

case $build_dir in
  /*) build=$build_dir ;;
  *) build=$(pwd)/$build_dir ;;
esac
install_prefix=$build/install

set -- \
  --wrap-mode=nofallback \
  --fatal-meson-warnings \
  --prefix="$install_prefix" \
  --libdir=lib \
  -Ddefault_library="$link_mode" \
  -Dlink_mode="$link_mode" \
  -Dtests=enabled \
  -Dwerror=true \
  "$@"

if [ -f "$build/meson-private/coredata.dat" ]; then
  meson setup --wipe "$build" "$@"
else
  meson setup "$build" "$@"
fi

meson compile -C "$build"
meson test -C "$build" --no-rebuild --print-errorlogs
printf '%s\n' "$install_prefix" >"$build/ci-install-prefix"
