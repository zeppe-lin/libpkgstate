#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu

source_root=$1
workflow=$source_root/.github/workflows/ci.yml

fail()
{
  echo "ci-qualification-source-test: $*" >&2
  exit 1
}

for file in \
  "$workflow" \
  "$source_root/ci/configure-and-test.sh" \
  "$source_root/ci/qualify-installed.sh" \
  "$source_root/ci/lint-manpages.sh" \
  "$source_root/ci/installed-core-consumer.cpp" \
  "$source_root/ci/installed-source-consumer.cpp" \
  "$source_root/ci/installed-build-consumer.cpp" \
  "$source_root/ci/installed-plan-consumer.cpp" \
  "$source_root/ci/installed-apply-consumer.cpp"
do
  test -s "$file" || fail "missing or empty ${file#"$source_root"/}"
done

for script in "$source_root"/ci/*.sh; do
  sh -n "$script" || fail "invalid shell syntax in ${script#"$source_root"/}"
done

for pin in \
  d9f6090beb65b1069ac49e02e010ceb2ea676ea5 \
  e1f6dfd8cc4bfeb2f8da44345d8ec6368281c6e0 \
  4fbe346fb08459ca56b0096eb54505130ea17244 \
  57a10b166450dd0396d4d461d1d38352073a5a1e \
  fbf21697f1fe4a597a07a6ffea3521af6e2d06e4
do
  count=$(grep -F -c "$pin" "$workflow")
  test "$count" -eq 3 ||
    fail "workflow must pin dependency $pin in all three jobs (found $count)"
done

for contract in \
  'GCC shared' \
  'GCC static' \
  'Clang shared' \
  'Clang static' \
  'address,undefined' \
  'meson==1.6.1' \
  'qualify-installed.sh' \
  'lint-manpages.sh'
do
  grep -F "$contract" "$workflow" >/dev/null ||
    fail "workflow omits $contract qualification"
done

for contract in \
  '--wrap-mode=nofallback' \
  '-Dsource_adapter=enabled' \
  '-Dbuild_adapter=enabled' \
  '-Dplanner_adapter=enabled' \
  '-Dapplication_adapter=enabled' \
  '-Dtools=enabled' \
  '-Dinstall_tools=true' \
  '-Dwerror=true'
do
  grep -F -- "$contract" "$source_root/ci/configure-and-test.sh" >/dev/null ||
    fail "configure entry point omits $contract"
done

grep -F 'core libpkgstate metadata is contaminated' \
  "$source_root/ci/qualify-installed.sh" >/dev/null ||
  fail 'installed qualification omits core dependency isolation'
grep -F 'pkgstate-check' "$source_root/ci/qualify-installed.sh" >/dev/null ||
  fail 'installed qualification omits diagnostic tool'
for page in \
  pkgstate_installation_receipt.3 \
  pkgstate_source_adapter.3 \
  pkgstate_build_adapter.3 \
  pkgstate_plan_adapter.3 \
  pkgstate_apply_adapter.3
do
  grep -F "$page" "$source_root/ci/lint-manpages.sh" >/dev/null ||
    fail "manual qualification omits $page"
done

test ! -e "$source_root/.github/workflows/build.yml" ||
  fail 'obsolete unpinned build workflow remains'
