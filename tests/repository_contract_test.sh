#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
fail(){ echo "repository-contract: $*" >&2; exit 1; }
for file in README.md HISTORY.md CONTRIBUTING.md MAINTAINING.md docs/architecture.md docs/integration.md docs/testing.md docs/code-style.md docs/history/3.0-adapter-extraction.md .clang-format .editorconfig; do [ -s "$root/$file" ] || fail "missing $file"; done
for script in "$root"/ci/*.sh "$root"/tests/*.sh; do sh -n "$script" || fail "invalid shell: ${script#$root/}"; done
if find "$root" -path "$root/.git" -prune -o -type f -print | grep -E '/(build|__pycache__)/|\.pyc$' >/dev/null; then fail 'generated build product tracked'; fi
for file in abi/libpkgstate.exports include/libpkgstate/export.h tools/generate-elf-export-script.sh tests/check_abi_surface.sh ci/audit-shared-boundary.sh; do [ -s "$root/$file" ] || fail "missing $file"; done

test -x "$root/tests/style_contract_test.sh" || fail 'style contract is absent'
