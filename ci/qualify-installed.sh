#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu

usage()
{
  echo "usage: qualify-installed.sh BUILD-DIR {shared|static}" >&2
  exit 2
}

[ "$#" -eq 2 ] || usage

build_dir=$1
link_mode=$2
case $link_mode in
  shared|static) ;;
  *) usage ;;
esac

script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)

for marker in ci-dependency-prefix ci-install-prefix; do
  [ -f "$build_dir/$marker" ] || {
    echo "build directory has no $marker" >&2
    exit 1
  }
done

dependency_prefix=$(cat "$build_dir/ci-dependency-prefix")
install_prefix=$(cat "$build_dir/ci-install-prefix")
case $build_dir in
  /*) build_path=$build_dir ;;
  *) build_path=$(pwd)/$build_dir ;;
esac

[ "$dependency_prefix" = "$build_path/dependencies" ] || {
  echo "refusing unexpected dependency prefix '$dependency_prefix'" >&2
  exit 1
}
[ "$install_prefix" = "$build_path/install" ] || {
  echo "refusing unexpected installation prefix '$install_prefix'" >&2
  exit 1
}

rm -rf "$install_prefix"
meson install -C "$build_dir"

temporary=$(mktemp -d "${TMPDIR:-/tmp}/libpkgstate-consumer.XXXXXX")
cleanup()
{
  chmod -R u+w "$temporary" 2>/dev/null || true
  rm -rf "$temporary"
}
trap cleanup EXIT HUP INT TERM

export PKG_CONFIG_PATH=$install_prefix/lib/pkgconfig:$dependency_prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}
unset PKG_CONFIG_SYSROOT_DIR
runtime_path=$install_prefix/lib:$dependency_prefix/lib

version=$(pkg-config --modversion libpkgstate)
[ "$version" = 0.3.0 ] || {
  echo "installed libpkgstate version is '$version', expected '0.3.0'" >&2
  exit 1
}
adapter_version=$(pkg-config --modversion libpkgstate-plan)
[ "$adapter_version" = 0.3.0 ] || {
  echo "installed adapter version is '$adapter_version', expected '0.3.0'" >&2
  exit 1
}

if {
  pkg-config --print-requires libpkgstate
  pkg-config --print-requires-private libpkgstate
} | grep -E '(^|[[:space:]])libpkg(image|plan)([[:space:]]|$)' >/dev/null; then
  echo 'core libpkgstate metadata is contaminated by image or planner dependencies' >&2
  exit 1
fi
pkg-config --print-requires libpkgstate-plan | grep -F 'libpkgstate = 0.3.0' >/dev/null || {
  echo 'adapter metadata omits exact libpkgstate dependency' >&2
  exit 1
}
pkg-config --print-requires libpkgstate-plan | grep -F 'libpkgplan >= 0.1.0' >/dev/null || {
  echo 'adapter metadata omits libpkgplan dependency floor' >&2
  exit 1
}

cxx=${CXX:-c++}
core_consumer=$temporary/core-consumer
plan_consumer=$temporary/plan-consumer

case $link_mode in
  shared)
    core_flags=$(pkg-config --cflags --libs libpkgstate)
    plan_flags=$(pkg-config --cflags --libs libpkgstate-plan)
    ;;
  static)
    core_flags=$(pkg-config --static --cflags --libs libpkgstate)
    plan_flags=$(pkg-config --static --cflags --libs libpkgstate-plan)
    ;;
esac

# shellcheck disable=SC2086
"$cxx" -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  "$script_dir/installed-core-consumer.cpp" $core_flags -o "$core_consumer"
# shellcheck disable=SC2086
"$cxx" -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  "$script_dir/installed-plan-consumer.cpp" $plan_flags -o "$plan_consumer"

for header in "$install_prefix"/include/libpkgstate/*.h; do
  unit=$temporary/$(basename "$header").cpp
  printf '#include <libpkgstate/%s>\n' "$(basename "$header")" >"$unit"
  # shellcheck disable=SC2046
  "$cxx" -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsyntax-only \
    $(pkg-config --cflags libpkgstate) "$unit"
done

unit=$temporary/adapter.cpp
printf '#include <libpkgstate-plan/adapter.h>\n' >"$unit"
# shellcheck disable=SC2046
"$cxx" -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsyntax-only \
  $(pkg-config --cflags libpkgstate-plan) "$unit"

canonical_store=$temporary/canonical
LD_LIBRARY_PATH=$runtime_path${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} \
  "$core_consumer" "$canonical_store"
LD_LIBRARY_PATH=$runtime_path${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} \
  "$plan_consumer"

identity()
{
  digit=$1
  printf 'v1:sha256:'
  printf '%064d' 0 | tr 0 "$digit"
}

LD_LIBRARY_PATH=$runtime_path${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} \
  "$install_prefix/bin/pkgstate-check" \
    --canonical-store "$canonical_store" \
    --managed-target "$(identity 1)" \
    --state-store "$(identity 2)" \
    --root-view "$(identity 3)" \
    --state-backend "$(identity 4)" \
    --publication-domain "$(identity 5)" \
    >"$temporary/canonical-report"
grep -F 'mode=canonical' "$temporary/canonical-report" >/dev/null
grep -F 'packages=0' "$temporary/canonical-report" >/dev/null

legacy_root=$temporary/legacy-root
mkdir -p "$legacy_root/var/lib/pkg"
cat >"$legacy_root/var/lib/pkg/db" <<'DATABASE'
alpha
1-1
usr/bin/alpha

DATABASE
LD_LIBRARY_PATH=$runtime_path${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} \
  "$install_prefix/bin/pkginfo" -r "$legacy_root" -i \
  >"$temporary/pkginfo-installed"
printf 'alpha 1-1\n' >"$temporary/pkginfo-expected"
cmp -s "$temporary/pkginfo-expected" "$temporary/pkginfo-installed"
LD_LIBRARY_PATH=$runtime_path${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} \
  "$install_prefix/bin/pkgstate-check" \
    --legacy-database "$legacy_root/var/lib/pkg/db" \
    >"$temporary/legacy-report"
grep -F 'mode=legacy' "$temporary/legacy-report" >/dev/null
grep -F 'packages=1' "$temporary/legacy-report" >/dev/null
LD_LIBRARY_PATH=$runtime_path${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} \
  "$install_prefix/bin/pkginfo" --version |
  grep -E '^pkginfo \(libpkgstate\) 0\.3\.0$' >/dev/null
LD_LIBRARY_PATH=$runtime_path${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} \
  "$install_prefix/bin/pkgstate-check" --version |
  grep -E '^pkgstate-check \(libpkgstate\) 0\.3\.0$' >/dev/null

if [ -s "$build_dir/man/libpkgstate.3" ]; then
  for page in \
    man1/pkginfo.1 \
    man1/pkgstate-check.1 \
    man3/libpkgstate.3 \
    man3/pkgstate_model.3 \
    man3/pkgstate_publication.3 \
    man7/pkgstate_authority.7
  do
    [ -s "$install_prefix/share/man/$page" ] || {
      echo "installed manual is absent: $page" >&2
      exit 1
    }
  done
fi

case $link_mode in
  shared)
    core_library=$(find "$install_prefix/lib" -maxdepth 1 -type f \
      -name 'libpkgstate.so.*' -print | sort | head -n 1)
    adapter_library=$(find "$install_prefix/lib" -maxdepth 1 -type f \
      -name 'libpkgstate-plan.so.*' -print | sort | head -n 1)
    [ -n "$core_library" ] || {
      echo 'installed shared libpkgstate library is absent' >&2
      exit 1
    }
    [ -n "$adapter_library" ] || {
      echo 'installed shared planner adapter library is absent' >&2
      exit 1
    }
    readelf -d "$core_library" | grep -q 'libcrypto\.so\.' || {
      echo 'shared libpkgstate does not record libcrypto' >&2
      exit 1
    }
    if readelf -d "$core_library" | grep -E 'libpkg(image|plan)\.so\.' >/dev/null; then
      echo 'shared libpkgstate is contaminated by image or planner linkage' >&2
      exit 1
    fi
    readelf -d "$adapter_library" | grep -q 'libpkgstate\.so\.' || {
      echo 'shared adapter does not record libpkgstate' >&2
      exit 1
    }
    readelf -d "$adapter_library" | grep -q 'libpkgplan\.so\.' || {
      echo 'shared adapter does not record libpkgplan' >&2
      exit 1
    }
    ;;
  static)
    [ -f "$install_prefix/lib/libpkgstate.a" ] || {
      echo 'installed static libpkgstate archive is absent' >&2
      exit 1
    }
    [ -f "$install_prefix/lib/libpkgstate-plan.a" ] || {
      echo 'installed static planner adapter archive is absent' >&2
      exit 1
    }
    pkg-config --static --libs libpkgstate >/dev/null
    pkg-config --static --libs libpkgstate-plan >/dev/null
    ;;
esac
