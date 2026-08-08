#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
fail(){ echo "repository-contract: $*" >&2; exit 1; }
for file in README.md HISTORY.md CONTRIBUTING.md MAINTAINING.md docs/architecture.md docs/integration.md docs/testing.md docs/code-style.md docs/abi.md docs/meson.build docs/history/3.0-adapter-extraction.md .clang-format .editorconfig; do [ -s "$root/$file" ] || fail "missing $file"; done
for script in "$root"/ci/*.sh "$root"/tests/contracts/*.sh; do sh -n "$script" || fail "invalid shell: ${script#$root/}"; done
if command -v git >/dev/null 2>&1 && git -C "$root" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  if git -C "$root" ls-files | grep -E '(^|/)(build|__pycache__)(/|$)|\.pyc$' >/dev/null; then
    fail 'generated build product tracked'
  fi
fi
for file in abi/libpkgstate.exports include/libpkgstate/export.h tools/generate-elf-export-script.sh tests/contracts/check_abi_surface.sh ci/audit-shared-boundary.sh; do [ -s "$root/$file" ] || fail "missing $file"; done

test -x "$root/tests/contracts/style_contract_test.sh" || fail 'style contract is absent'

test -x "$root/tools/check-public-documentation.py" || fail 'public documentation checker is absent'
test -x "$root/tools/check-doxygen-contract.py" || fail 'Doxygen AST contract checker is absent'

for tool in \
  build-html-docs.py check-html-docs.py install-html-docs.py \
  render-man-markdown.py check-man-markdown.py check-html-manifest.py; do
  test -x "$root/tools/$tool" || fail "missing executable tools/$tool"
done

for helper in \
  ci/qualify-html-docs.sh ci/qualify-installed-documentation.py; do
  test -x "$root/$helper" || fail "missing executable $helper"
done
