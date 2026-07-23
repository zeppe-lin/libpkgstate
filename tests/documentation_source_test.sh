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
  man/pkgstate_canonical_generation_store.3.scdoc \
  PKGSTATE_CANONICAL_GENERATION_STORE\(3\)
check_page \
  man/pkgstate_legacy_compatibility.3.scdoc \
  PKGSTATE_LEGACY_COMPATIBILITY\(3\)
check_page \
  man/pkgstate_legacy_import.3.scdoc \
  PKGSTATE_LEGACY_IMPORT\(3\)
check_page \
  man/pkgstate_plan_adapter.3.scdoc \
  PKGSTATE_PLAN_ADAPTER\(3\)
check_page \
  man/pkgstate_legacy_text_store.3.scdoc \
  PKGSTATE_LEGACY_TEXT_STORE\(3\)
check_page man/pkgstate-db.5.scdoc PKGSTATE-DB\(5\)
check_page man/pkgstate-generation.5.scdoc PKGSTATE-GENERATION\(5\)
check_page man/pkginfo.1.scdoc PKGINFO\(1\)
check_page man/pkgstate-check.1.scdoc PKGSTATE-CHECK\(1\)

grep -F 'A package-state transaction is not a filesystem transaction.' \
  "$source_root/man/pkgstate_store.3.scdoc" >/dev/null ||
  fail "store manual omits filesystem-transaction boundary"

grep -F '*compare_and_publish()* is non-virtual.' \
  "$source_root/man/pkgstate_store.3.scdoc" >/dev/null ||
  fail "store manual omits non-overridable comparison"

grep -F 'without invoking publication when they differ.' \
  "$source_root/man/pkgstate_store.3.scdoc" >/dev/null ||
  fail "store manual omits stale no-publication contract"


grep -F 'Only the _current_ selector chooses authoritative state.' \
  "$source_root/man/pkgstate_canonical_generation_store.3.scdoc" >/dev/null ||
  fail "generation backend manual omits selector authority"

grep -F 'writes _binding_ last as the initialization-complete' \
  "$source_root/man/pkgstate_canonical_generation_store.3.scdoc" >/dev/null ||
  fail "generation backend manual omits initialization marker"

grep -F '*immutable_generation_selection*.' \
  "$source_root/man/pkgstate_canonical_generation_store.3.scdoc" >/dev/null ||
  fail "generation backend manual omits atomicity boundary"

grep -F 'does not maintain a' \
  "$source_root/man/pkgstate_canonical_generation_store.3.scdoc" >/dev/null ||
  fail "generation backend manual omits receipt-journal boundary"

grep -F '*open_existing()* is the read-only opening path.' \
  "$source_root/man/pkgstate_canonical_generation_store.3.scdoc" >/dev/null ||
  fail "generation backend manual omits non-initializing open"

grep -F 'The command is read-only.' \
  "$source_root/man/pkgstate-check.1.scdoc" >/dev/null ||
  fail "diagnostic manual omits read-only boundary"

grep -F 'never invokes the' \
  "$source_root/man/pkgstate-check.1.scdoc" >/dev/null ||
  fail "diagnostic manual omits non-initializing canonical path"

grep -F 'It does not parse opaque' \
  "$source_root/man/pkgstate-check.1.scdoc" >/dev/null ||
  fail "diagnostic manual permits legacy reconstruction"

grep -F 'non-blocking' \
  "$source_root/man/pkgstate_legacy_text_store.3.scdoc" >/dev/null ||
  fail "legacy backend manual omits non-blocking lock semantics"

grep -F 'db.backup' \
  "$source_root/man/pkgstate_legacy_text_store.3.scdoc" >/dev/null ||
  fail "legacy backend manual omits backup contract"

grep -F 'A blank line or end-of-file terminates' \
  "$source_root/man/pkgstate-db.5.scdoc" >/dev/null ||
  fail "database manual omits record terminator"


grep -F '"pkgstate-snapshot\0"' \
  "$source_root/man/pkgstate-generation.5.scdoc" >/dev/null ||
  fail "generation format manual omits snapshot magic"

grep -F 're-encodes the snapshot' \
  "$source_root/man/pkgstate-generation.5.scdoc" >/dev/null ||
  fail "generation format manual omits canonical re-encoding"

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

grep -F 'Callers cannot attach arbitrary ownership or' \
  "$source_root/man/pkgstate_model.3.scdoc" >/dev/null ||
  fail "model manual omits snapshot identity authority"


grep -F 'The request is immutable and does not mutate storage.' \
  "$source_root/man/pkgstate_model.3.scdoc" >/dev/null ||
  fail "model manual omits immutable publication boundary"

grep -F 'requires *transaction_evidence_identity*' \
  "$source_root/man/pkgstate_model.3.scdoc" >/dev/null ||
  fail "model manual omits composed request evidence"

grep -F 'A stale-state receipt is an ordinary typed outcome.' \
  "$source_root/man/pkgstate_model.3.scdoc" >/dev/null ||
  fail "model manual omits typed stale-state outcome"

grep -F 'it does not acquire a lock,' \
  "$source_root/man/pkgstate_model.3.scdoc" >/dev/null ||
  fail "model manual makes receipt construction a publication operation"

grep -F 'caller-authored complete replacement snapshot.' \
  "$source_root/DESIGN.md" >/dev/null ||
  fail "design permits caller-authored replacement snapshots"

grep -F 'path-ordered ownership groups' \
  "$source_root/DESIGN.md" >/dev/null ||
  fail "design omits canonical ownership identity ordering"

grep -F 'legacy_snapshot read() const;' \
  "$source_root/man/pkgstate_store.3.scdoc" >/dev/null ||
  fail "store manual does not expose compatibility snapshot type"

grep -F 'constructs *legacy_installed_package* records' \
  "$source_root/man/pkgstate_legacy_text_store.3.scdoc" >/dev/null ||
  fail "legacy backend manual claims canonical installed records"


grep -F 'derived_from_retained_facts' \
  "$source_root/man/pkgstate_legacy_compatibility.3.scdoc" >/dev/null ||
  fail "legacy compatibility manual omits derived fact origin"

grep -F 'legacy_snapshot_observation_identity' \
  "$source_root/man/pkgstate_legacy_compatibility.3.scdoc" >/dev/null ||
  fail "legacy compatibility manual omits observation identity"

grep -F 'The opaque legacy version line is never parsed.' \
  "$source_root/man/pkgstate_legacy_import.3.scdoc" >/dev/null ||
  fail "legacy import manual permits version-line guessing"

grep -F 'requires one empty canonical destination' \
  "$source_root/man/pkgstate_legacy_import.3.scdoc" >/dev/null ||
  fail "legacy import manual omits fresh-destination boundary"

grep -F 'does not invoke the backend publication primitive' \
  "$source_root/man/pkgstate_legacy_import.3.scdoc" >/dev/null ||
  fail "legacy import manual omits stale no-publication contract"

grep -F 'No overload accepts *legacy_snapshot*.' \
  "$source_root/man/pkgstate_plan_adapter.3.scdoc" >/dev/null ||
  fail "planner adapter manual permits incomplete legacy projection"

grep -F 'does not invent partial filesystem metadata' \
  "$source_root/man/pkgstate_plan_adapter.3.scdoc" >/dev/null ||
  fail "planner adapter manual permits invented object metadata"

grep -F 'An empty retained manifest is known empty' \
  "$source_root/DESIGN.md" >/dev/null ||
  fail "design collapses known-empty legacy ownership into unavailable"

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
  'pkgstate_canonical_generation_store(3)' \
  'pkgstate_legacy_compatibility(3)' \
  'pkgstate_legacy_import(3)' \
  'pkgstate_plan_adapter(3)' \
  'pkgstate_legacy_text_store(3)' \
  'pkgstate-db(5)' \
  'pkgstate-generation(5)' \
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
