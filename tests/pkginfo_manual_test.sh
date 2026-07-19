#!/bin/sh
# Copyright (C) 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu

source_root=$1
pkginfo=$2
manual=$source_root/man/pkginfo.1.scdoc
work=$(mktemp -d "${TMPDIR:-/tmp}/libpkgstate-pkginfo-manual.XXXXXX")
trap 'rm -rf "$work"' EXIT HUP INT TERM

fail()
{
  echo "pkginfo-manual-test: $*" >&2
  exit 1
}

if ! "$pkginfo" --help >"$work/help" 2>"$work/stderr"; then
  cat "$work/stderr" >&2
  fail "pkginfo --help failed"
fi

test ! -s "$work/stderr" || {
  cat "$work/stderr" >&2
  fail "pkginfo --help wrote to stderr"
}

for option in \
  '--installed' \
  '--list' \
  '--owner' \
  '--root' \
  '--version' \
  '--help'
do
  grep -F -- "$option" "$work/help" >/dev/null ||
    fail "help omits $option"
  grep -F -- "$option" "$manual" >/dev/null ||
    fail "manual omits $option"
done

for status in 0 1 2
do
  grep -F "*$status*" "$manual" >/dev/null ||
    fail "manual omits status $status"
done

grep -F 'name version' "$manual" >/dev/null ||
  fail "manual omits installed and owner output contract"

grep -F 'trailing slash' "$manual" >/dev/null ||
  fail "manual omits directory output contract"

grep -F 'exact ownership lookup' "$manual" >/dev/null ||
  fail "manual omits exact ownership semantics"
