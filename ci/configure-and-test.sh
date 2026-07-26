#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
usage() { echo "usage: configure-and-test.sh BUILD-DIR {shared|static} [MESON-OPTION ...]" >&2; exit 2; }
[ "$#" -ge 2 ] || usage
build_dir=$1
link_mode=$2
shift 2
case $link_mode in shared|static) ;; *) usage ;; esac
script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
source_dir=$(CDPATH= cd "$script_dir/.." && pwd)
pkgsource_source=${LIBPKGSOURCE_SOURCE:-$source_dir/subprojects/libpkgsource}
image_source=${LIBPKGIMAGE_SOURCE:-$source_dir/subprojects/libpkgimage}
build_source=${LIBPKGBUILD_SOURCE:-$source_dir/subprojects/libpkgbuild}
plan_source=${LIBPKGPLAN_SOURCE:-$source_dir/subprojects/libpkgplan}
apply_source=${LIBPKGAPPLY_SOURCE:-$source_dir/subprojects/libpkgapply}
for pair in \
  "$pkgsource_source:libpkgsource" \
  "$image_source:libpkgimage" \
  "$build_source:libpkgbuild" \
  "$plan_source:libpkgplan" \
  "$apply_source:libpkgapply"
do
  path=${pair%:*} name=${pair#*:}
  [ -f "$path/meson.build" ] || { echo "$name source is absent: $path" >&2; exit 1; }
done
case $build_dir in /*) build_path=$build_dir ;; *) build_path=$(pwd)/$build_dir ;; esac
dependency_prefix=$build_path/dependencies
install_prefix=$build_path/install
rm -rf "$dependency_prefix" "$install_prefix"
mkdir -p "$build_path"
setup()
{
  src=$1 out=$2
  shift 2
  if [ -f "$out/meson-private/coredata.dat" ]; then
    meson setup --wipe "$out" "$src" "$@"
  else
    meson setup "$out" "$src" "$@"
  fi
}
configure_dependency()
{
  src=$1 out=$2
  setup "$src" "$out" \
    --wrap-mode=nofallback --fatal-meson-warnings \
    --prefix="$dependency_prefix" --libdir=lib \
    -Ddefault_library="$link_mode" -Dlink_mode="$link_mode" \
    -Dtests=disabled -Dman_pages=disabled -Dwerror=true
  meson compile -C "$out"
  meson install -C "$out"
}
configure_dependency "$pkgsource_source" "$build_path/libpkgsource"
configure_dependency "$image_source" "$build_path/libpkgimage"
configure_dependency "$build_source" "$build_path/libpkgbuild"
export PKG_CONFIG_PATH=$dependency_prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}
unset PKG_CONFIG_SYSROOT_DIR
export LD_LIBRARY_PATH=$dependency_prefix/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
configure_dependency "$plan_source" "$build_path/libpkgplan"
configure_dependency "$apply_source" "$build_path/libpkgapply"
set -- \
  --wrap-mode=nofallback --fatal-meson-warnings \
  --prefix="$install_prefix" --libdir=lib \
  -Ddefault_library="$link_mode" -Dlink_mode="$link_mode" \
  -Dtests=enabled -Dtools=enabled -Dinstall_tools=true \
  -Dsource_adapter=enabled -Dbuild_adapter=enabled \
  -Dplanner_adapter=enabled -Dapplication_adapter=enabled -Dwerror=true "$@"
if [ -f "$build_dir/meson-private/coredata.dat" ]; then
  meson setup --wipe "$build_dir" "$@"
else
  meson setup "$build_dir" "$@"
fi
meson compile -C "$build_dir"
meson test -C "$build_dir" --no-rebuild --print-errorlogs
printf '%s\n' "$dependency_prefix" >"$build_dir/ci-dependency-prefix"
printf '%s\n' "$install_prefix" >"$build_dir/ci-install-prefix"
