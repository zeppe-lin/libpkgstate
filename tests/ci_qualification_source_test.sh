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
  "$source_root/ci/installed-plan-consumer.cpp"
do
  test -s "$file" || fail "missing or empty ${file#"$source_root"/}"
done

for script in "$source_root"/ci/*.sh; do
  sh -n "$script" || fail "invalid shell syntax in ${script#"$source_root"/}"
done

for pin in \
  e1f6dfd8cc4bfeb2f8da44345d8ec6368281c6e0 \
  a5dfd231aee8ed39175a5a366173a50b7bd3374a
do
  grep -F "$pin" "$workflow" >/dev/null ||
    fail "workflow omits dependency pin $pin"
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
  '-Dplanner_adapter=enabled' \
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
grep -F 'pkgstate_plan_adapter.3' "$source_root/ci/lint-manpages.sh" >/dev/null ||
  fail 'manual qualification omits planner adapter page'
