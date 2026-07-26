#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
root=$1
fail() { echo "documentation-source-test: $*" >&2; exit 1; }
check_page()
{
  file=$1 heading=$2
  test -s "$root/$file" || fail "missing $file"
  test "$(sed -n '1p' "$root/$file")" = "$heading" ||
    fail "$file has wrong heading"
}
check_page man/libpkgstate.3.scdoc 'LIBPKGSTATE(3)'
check_page man/pkgstate_authority.7.scdoc 'PKGSTATE_AUTHORITY(7)'
check_page man/pkgstate_model.3.scdoc 'PKGSTATE_MODEL(3)'
check_page man/pkgstate_installation_receipt.3.scdoc 'PKGSTATE_INSTALLATION_RECEIPT(3)'
check_page man/pkgstate_publication.3.scdoc 'PKGSTATE_PUBLICATION(3)'
check_page man/pkgstate_store.3.scdoc 'PKGSTATE_STORE(3)'
check_page man/pkgstate_canonical_generation_store.3.scdoc 'PKGSTATE_CANONICAL_GENERATION_STORE(3)'
check_page man/pkgstate_source_adapter.3.scdoc 'PKGSTATE_SOURCE_ADAPTER(3)'
check_page man/pkgstate_build_adapter.3.scdoc 'PKGSTATE_BUILD_ADAPTER(3)'
check_page man/pkgstate_plan_adapter.3.scdoc 'PKGSTATE_PLAN_ADAPTER(3)'
check_page man/pkgstate_apply_adapter.3.scdoc 'PKGSTATE_APPLY_ADAPTER(3)'
check_page man/pkgstate-generation.5.scdoc 'PKGSTATE-GENERATION(5)'
check_page man/pkgstate-check.1.scdoc 'PKGSTATE-CHECK(1)'
corpus=$(
  { cat "$root"/*.md; find "$root/man" -type f -name '*.scdoc' -exec cat {} +; } |
    tr '\n\t' '  ' | tr -s ' ' )
for contract in \
  'Package release identity remains source-owned.' \
  'There is no incomplete native installed-package record.' \
  'The backend cannot silently rebase an old request' \
  'The receipt is not reconstructed from a package filename' \
  'The authoritative library contains no old-format parser' \
  'Upgrade preserves the prior installed reason.' \
  'The YAML document is not authority at this boundary.' \
  'Build authority is admitted only after exact artifact inspection.' \
  'Application admission never reconstructs build provenance from planner facts.' \
  'It accepts no second caller-supplied build authority.' \
  'Generation-v3 storage does not change in this release.' \
  'Native publication begins with a fresh state target.'
do
  printf '%s\n' "$corpus" | grep -F "$contract" >/dev/null ||
    fail "documentation omits: $contract"
done
for removed in \
  man/pkgstate_legacy_compatibility.3.scdoc \
  man/pkgstate_legacy_import.3.scdoc \
  man/pkgstate_legacy_text_store.3.scdoc \
  man/pkgstate-db.5.scdoc \
  man/pkginfo.1.scdoc
do
  test ! -e "$root/$removed" || fail "removed contract remains: $removed"
done
