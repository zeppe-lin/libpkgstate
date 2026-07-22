#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu

source_root=$1

fail()
{
  echo "documentation-source-test: $*" >&2
  exit 1
}

check_page()
{
  relative=$1
  heading=$2
  page=$source_root/$relative

  test -s "$page" || fail "missing or empty $relative"
  first=$(sed -n '1p' "$page")
  test "$first" = "$heading" ||
    fail "$relative heading is '$first', expected '$heading'"
}

check_page man/libpkgstate.3.scdoc LIBPKGSTATE\(3\)
check_page man/pkgstate_model.3.scdoc PKGSTATE_MODEL\(3\)
check_page man/pkgstate_store.3.scdoc PKGSTATE_STORE\(3\)
check_page \
  man/pkgstate_legacy_text_store.3.scdoc \
  PKGSTATE_LEGACY_TEXT_STORE\(3\)
check_page man/pkgstate-db.5.scdoc PKGSTATE-DB\(5\)
check_page man/pkginfo.1.scdoc PKGINFO\(1\)

grep -F 'A package-state transaction is not a filesystem transaction.' \
  "$source_root/man/pkgstate_store.3.scdoc" >/dev/null ||
  fail "store manual omits filesystem-transaction boundary"

grep -F 'non-blocking' \
  "$source_root/man/pkgstate_legacy_text_store.3.scdoc" >/dev/null ||
  fail "legacy backend manual omits non-blocking lock semantics"

grep -F 'db.backup' \
  "$source_root/man/pkgstate_legacy_text_store.3.scdoc" >/dev/null ||
  fail "legacy backend manual omits backup contract"

grep -F 'A blank line or end-of-file terminates' \
  "$source_root/man/pkgstate-db.5.scdoc" >/dev/null ||
  fail "database manual omits record terminator"

grep -F 'Shared ownership is valid state.' \
  "$source_root/man/pkgstate_model.3.scdoc" >/dev/null ||
  fail "model manual omits shared ownership"

grep -F 'v1:sha256:<64 lowercase hexadecimal digits>' \
  "$source_root/man/pkgstate_model.3.scdoc" >/dev/null ||
  fail "model manual omits canonical digest representation"

grep -F 'cannot attach arbitrary identity bytes to different coordinates.' \
  "$source_root/man/pkgstate_model.3.scdoc" >/dev/null ||
  fail "model manual omits package-release identity authority"

grep -F 'Known empty and historically unavailable are different states.' \
  "$source_root/man/pkgstate_model.3.scdoc" >/dev/null ||
  fail "model manual omits installed-control completeness"

grep -F 'A pathname is a locator, not a target identity.' \
  "$source_root/man/pkgstate_model.3.scdoc" >/dev/null ||
  fail "model manual omits target-binding identity boundary"

grep -F 'complete canonical installed truth for one' \
  "$source_root/man/pkgstate_model.3.scdoc" >/dev/null ||
  fail "model manual omits complete installed-package boundary"

grep -F 'legacy_snapshot read() const;' \
  "$source_root/man/pkgstate_store.3.scdoc" >/dev/null ||
  fail "store manual does not expose compatibility snapshot type"

grep -F 'constructs *legacy_installed_package* records' \
  "$source_root/man/pkgstate_legacy_text_store.3.scdoc" >/dev/null ||
  fail "legacy backend manual claims canonical installed records"

for document in DESIGN.md STORAGE.md MIGRATION.md TESTING.md HISTORY.md
do
  test -s "$source_root/$document" || fail "missing or empty $document"
  grep -F "$document" "$source_root/Doxyfile" >/dev/null ||
    fail "Doxyfile omits $document"
done

for page in \
  'libpkgstate(3)' \
  'pkgstate_model(3)' \
  'pkgstate_store(3)' \
  'pkgstate_legacy_text_store(3)' \
  'pkgstate-db(5)' \
  'pkginfo(1)'
do
  grep -F "$page" "$source_root/README.md" >/dev/null ||
    fail "README omits $page"
done

grep -F 'The frontend may compose installed truth and archive truth' \
  "$source_root/DESIGN.md" >/dev/null ||
  fail "design omits frontend composition boundary"

grep -F 'Historical CRUX `pkgmk` calls `pkginfo -f`' \
  "$source_root/MIGRATION.md" >/dev/null ||
  fail "migration omits footprint transition gate"

grep -F '`libpkgimage` 0.3.0 or later when reference tools are enabled' \
  "$source_root/README.md" >/dev/null ||
  fail "README omits optional libpkgimage dependency floor"
