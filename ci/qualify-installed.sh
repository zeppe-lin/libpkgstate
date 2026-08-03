#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
[ "$#" -eq 2 ] || exit 2
build_dir=$1; link_mode=$2
install_prefix=$(cat "$build_dir/ci-install-prefix")
rm -rf "$install_prefix"; meson install -C "$build_dir"
export PKG_CONFIG_PATH=$install_prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}
unset PKG_CONFIG_SYSROOT_DIR
test "$(pkg-config --modversion libpkgstate)" = 3.0.0
if { pkg-config --print-requires libpkgstate; pkg-config --print-requires-private libpkgstate; } | grep -E 'libpkg(source|build|image|plan|apply)|libpkgstate-' >/dev/null; then echo 'contaminated core metadata' >&2; exit 1; fi
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT HUP INT TERM
flags=$(pkg-config --cflags --libs libpkgstate); [ "$link_mode" = shared ] || flags=$(pkg-config --static --cflags --libs libpkgstate)
case $link_mode in
  shared) if printf '%s\n' "$flags" | grep -F -- '-lcrypto' >/dev/null; then echo 'private link edge leaked into shared consumer flags: -lcrypto' >&2; exit 1; fi ;;
  static) printf '%s\n' "$flags" | grep -F -- '-lcrypto' >/dev/null || { echo 'static link closure omits -lcrypto' >&2; exit 1; } ;;
esac
# shellcheck disable=SC2086
${CXX:-c++} -std=c++17 -Wall -Wextra -Wpedantic -Werror "$(dirname "$0")/installed-core-consumer.cpp" $flags -o "$tmp/consumer"
"$tmp/consumer"
for header in "$install_prefix"/include/libpkgstate/*.h; do printf '#include <libpkgstate/%s>
' "$(basename "$header")" >"$tmp/header.cpp"; ${CXX:-c++} -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsyntax-only $(pkg-config --cflags libpkgstate) "$tmp/header.cpp"; done
case $link_mode in
  shared) "$(dirname "$0")/audit-shared-boundary.sh" "$install_prefix/lib/libpkgstate.so.3.0.0" ;;
  static) test -f "$install_prefix/lib/libpkgstate.a" ;;
esac
if [ -d "$build_dir/man" ]; then
  for page in "$build_dir"/man/*.[1357]; do
    [ -e "$page" ] || continue
    section=${page##*.}
    test -s "$install_prefix/share/man/man$section/$(basename "$page")"
  done
fi
