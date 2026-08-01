#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
usage() { echo "usage: qualify-installed.sh BUILD-DIR {shared|static}" >&2; exit 2; }
[ "$#" -eq 2 ] || usage
build_dir=$1 link_mode=$2
case $link_mode in shared|static) ;; *) usage ;; esac
script_dir=$(CDPATH= cd "$(dirname "$0")" && pwd)
for marker in ci-dependency-prefix ci-install-prefix; do [ -f "$build_dir/$marker" ] || { echo "build directory has no $marker" >&2; exit 1; }; done
dependency_prefix=$(cat "$build_dir/ci-dependency-prefix")
install_prefix=$(cat "$build_dir/ci-install-prefix")
rm -rf "$install_prefix"
meson install -C "$build_dir"
temporary=$(mktemp -d "${TMPDIR:-/tmp}/libpkgstate-consumer.XXXXXX")
trap 'chmod -R u+w "$temporary" 2>/dev/null || :; rm -rf "$temporary"' EXIT HUP INT TERM
export PKG_CONFIG_PATH=$install_prefix/lib/pkgconfig:$dependency_prefix/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}
unset PKG_CONFIG_SYSROOT_DIR
runtime_path=$install_prefix/lib:$dependency_prefix/lib
for module in libpkgstate libpkgstate-source libpkgstate-build libpkgstate-plan libpkgstate-apply; do
  version=$(pkg-config --modversion "$module")
  [ "$version" = 2.4.0 ] || { echo "$module version is '$version', expected 2.4.0" >&2; exit 1; }
done
if { pkg-config --print-requires libpkgstate; pkg-config --print-requires-private libpkgstate; } |
  grep -E '(^|[[:space:]])libpkg(source|build|image|plan|apply)([[:space:]]|$)' >/dev/null; then
  echo 'core libpkgstate metadata is contaminated by external authority dependencies' >&2
  exit 1
fi
pkg-config --print-requires libpkgstate-source | grep -F 'libpkgsource >= 2.0.0' >/dev/null
pkg-config --print-requires libpkgstate-build | grep -F 'libpkgstate-source >= 2.3.0' >/dev/null
pkg-config --print-requires libpkgstate-build | grep -F 'libpkgbuild >= 2.0.0' >/dev/null
pkg-config --print-requires libpkgstate-build | grep -F 'libpkgimage >= 0.3.0' >/dev/null
pkg-config --print-requires libpkgstate-plan | grep -F 'libpkgplan >= 0.2.0' >/dev/null
pkg-config --print-requires libpkgstate-apply | grep -F 'libpkgstate-source >= 2.3.0' >/dev/null
pkg-config --print-requires libpkgstate-apply | grep -F 'libpkgstate-build >= 2.3.0' >/dev/null
pkg-config --print-requires libpkgstate-apply | grep -F 'libpkgapply >= 2.1.0' >/dev/null
cxx=${CXX:-c++}
for pair in \
  core:libpkgstate \
  source:libpkgstate-source \
  build:libpkgstate-build \
  plan:libpkgstate-plan \
  apply:libpkgstate-apply
do
  name=${pair%:*} module=${pair#*:}
  case $link_mode in
    shared) flags=$(pkg-config --cflags --libs "$module") ;;
    static) flags=$(pkg-config --static --cflags --libs "$module") ;;
  esac
  # shellcheck disable=SC2086
  "$cxx" -std=c++17 -Wall -Wextra -Wpedantic -Werror \
    "$script_dir/installed-$name-consumer.cpp" $flags \
    -o "$temporary/$name-consumer"
done
for header in "$install_prefix"/include/libpkgstate/*.h; do
  unit=$temporary/$(basename "$header").cpp
  printf '#include <libpkgstate/%s>\n' "$(basename "$header")" >"$unit"
  # shellcheck disable=SC2046
  "$cxx" -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsyntax-only \
    $(pkg-config --cflags libpkgstate) "$unit"
done
for module in source build plan apply; do
  unit=$temporary/$module.cpp
  printf '#include <libpkgstate-%s/adapter.h>\n' "$module" >"$unit"
  # shellcheck disable=SC2046
  "$cxx" -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsyntax-only \
    $(pkg-config --cflags "libpkgstate-$module") "$unit"
done
unit=$temporary/apply-state-projection.cpp
printf '#include <libpkgstate-apply/state_projection.h>\n' >"$unit"
# shellcheck disable=SC2046
"$cxx" -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsyntax-only \
  $(pkg-config --cflags libpkgstate-apply) "$unit"
canonical_store=$temporary/canonical
for consumer in core source build plan apply; do
  if [ "$consumer" = core ]; then
    LD_LIBRARY_PATH=$runtime_path${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} \
      "$temporary/$consumer-consumer" "$canonical_store"
  else
    LD_LIBRARY_PATH=$runtime_path${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} \
      "$temporary/$consumer-consumer"
  fi
done
identity()
{
  digit=$1
  printf 'v1:sha256:'
  printf '%064d' 0 | tr 0 "$digit"
}
LD_LIBRARY_PATH=$runtime_path${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} \
  "$install_prefix/bin/pkgstate-check" \
  --canonical-store "$canonical_store" \
  --managed-target "$(identity 1)" --state-store "$(identity 2)" \
  --root-view "$(identity 3)" --state-backend "$(identity 4)" \
  --publication-domain "$(identity 5)" >"$temporary/report"
grep -F 'storage-format=libpkgstate-generation-v3' "$temporary/report" >/dev/null
grep -F 'packages=0' "$temporary/report" >/dev/null
LD_LIBRARY_PATH=$runtime_path${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} \
  "$install_prefix/bin/pkgstate-check" --version |
  grep -E '^pkgstate-check \(libpkgstate\) 2\.4\.0$' >/dev/null
if [ -s "$build_dir/man/libpkgstate.3" ]; then
  for page in \
    man1/pkgstate-check.1 \
    man3/libpkgstate.3 \
    man3/pkgstate_model.3 \
    man3/pkgstate_installation_receipt.3 \
    man3/pkgstate_source_adapter.3 \
    man3/pkgstate_build_adapter.3 \
    man3/pkgstate_plan_adapter.3 \
    man3/pkgstate_apply_adapter.3 \
    man7/pkgstate_authority.7
  do
    [ -s "$install_prefix/share/man/$page" ] || { echo "installed manual is absent: $page" >&2; exit 1; }
  done
fi
case $link_mode in
  shared)
    for spec in \
      'pkgstate:3' \
      'pkgstate-source:1' \
      'pkgstate-build:1' \
      'pkgstate-plan:2' \
      'pkgstate-apply:3'
    do
      name=${spec%:*} soname=${spec#*:}
      library=$(find "$install_prefix/lib" -maxdepth 1 -type f -name "lib$name.so.*" -print | sort | head -n 1)
      [ -n "$library" ] || { echo "installed shared lib$name is absent" >&2; exit 1; }
      readelf -d "$library" | grep -E "SONAME.*\[lib$name\\.so\\.$soname\]" >/dev/null || {
        echo "shared lib$name SONAME is not $soname" >&2; exit 1;
      }
    done
    core=$(find "$install_prefix/lib" -maxdepth 1 -type f -name 'libpkgstate.so.*' -print | sort | head -n 1)
    if readelf -d "$core" | grep -E 'libpkg(source|build|image|plan|apply)\.so\.' >/dev/null; then
      echo 'shared core is contaminated by external authority linkage' >&2
      exit 1
    fi
    ;;
  static)
    for name in pkgstate pkgstate-source pkgstate-build pkgstate-plan pkgstate-apply; do
      [ -f "$install_prefix/lib/lib$name.a" ] || { echo "installed static lib$name is absent" >&2; exit 1; }
      pkg-config --static --libs "lib$name" >/dev/null
    done
    ;;
esac
