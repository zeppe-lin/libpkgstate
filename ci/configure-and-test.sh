#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
[ "$#" -ge 2 ] || { echo "usage: configure-and-test.sh BUILD-DIR {shared|static} [MESON-ARG...]" >&2; exit 2; }
build_dir=$1
link_mode=$2
shift 2
case $link_mode in shared|static) ;; *) exit 2 ;; esac
install_prefix=$(pwd)/$build_dir/install
set -- --wrap-mode=nofallback --fatal-meson-warnings --prefix="$install_prefix" --libdir=lib -Ddefault_library="$link_mode" -Dlink_mode="$link_mode" -Dtests=enabled -Dwerror=true "$@"
if [ -f "$build_dir/meson-private/coredata.dat" ]; then meson setup --wipe "$build_dir" "$@"; else meson setup "$build_dir" "$@"; fi
meson compile -C "$build_dir"
meson test -C "$build_dir" --no-rebuild --print-errorlogs
printf '%s
' "$install_prefix" >"$build_dir/ci-install-prefix"
