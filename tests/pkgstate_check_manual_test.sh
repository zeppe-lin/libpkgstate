#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu

source_root=$1
pkgstate_check=$2
manual=$source_root/man/pkgstate-check.1.scdoc

temporary=$(mktemp -d)
trap 'rm -rf "$temporary"' EXIT HUP INT TERM

fail()
{
  echo "pkgstate-check-manual-test: $*" >&2
  exit 1
}

test -s "$manual" || fail 'manual source is missing'
test "$(sed -n '1p' "$manual")" = 'PKGSTATE-CHECK(1)' ||
  fail 'manual heading is wrong'

"$pkgstate_check" --help > "$temporary/help" 2> "$temporary/error" ||
  fail 'help command failed'
test ! -s "$temporary/error" || fail 'help command wrote standard error'

for option in \
  '--canonical-store' \
  '--legacy-database' \
  '--managed-target' \
  '--state-store' \
  '--root-view' \
  '--state-backend' \
  '--publication-domain' \
  '--version' \
  '--help'
do
  grep -F -- "$option" "$temporary/help" >/dev/null ||
    fail "help omits $option"
  grep -F -- "$option" "$manual" >/dev/null ||
    fail "manual omits $option"
done

grep -F 'The command is read-only.' "$manual" >/dev/null ||
  fail 'manual omits read-only contract'
grep -F 'never invokes the' "$manual" >/dev/null ||
  fail 'manual omits non-initializing canonical open'
grep -F 'It does not parse opaque' "$manual" >/dev/null ||
  fail 'manual permits legacy version reconstruction'
grep -F 'Command-line usage is invalid.' "$manual" >/dev/null ||
  fail 'manual omits status 2'
grep -F 'does not produce a partial' "$manual" >/dev/null ||
  fail 'manual omits all-or-nothing report boundary'
