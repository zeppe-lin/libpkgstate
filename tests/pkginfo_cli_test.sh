#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu

pkginfo=$1
work=$(mktemp -d "${TMPDIR:-/tmp}/libpkgstate-pkginfo.XXXXXX")
trap 'rm -rf "$work"' EXIT HUP INT TERM

root=$work/root
mkdir -p "$root/var/lib/pkg"
cat > "$root/var/lib/pkg/db" <<'DBEOF'
alpha
1.0
etc/alpha.conf
usr/share/alpha/

beta
2.0
usr/bin/beta
usr/share/alpha/

empty
3.0

DBEOF

fail()
{
  echo "pkginfo-cli-test: $*" >&2
  exit 1
}

expect_success()
{
  name=$1
  shift
  if ! "$@" > "$work/actual" 2> "$work/stderr"; then
    cat "$work/stderr" >&2
    fail "$name failed"
  fi
  test ! -s "$work/stderr" || {
    cat "$work/stderr" >&2
    fail "$name wrote to stderr"
  }
  cmp "$work/expected" "$work/actual" || fail "$name output differs"
}

cat > "$work/expected" <<'EOF_INSTALLED'
alpha 1.0
beta 2.0
empty 3.0
EOF_INSTALLED
expect_success installed "$pkginfo" -r "$root" -i

cat > "$work/expected" <<'EOF_LIST'
etc/alpha.conf
usr/share/alpha/
EOF_LIST
expect_success list "$pkginfo" -r "$root" -l alpha

: > "$work/expected"
expect_success empty-manifest "$pkginfo" -r "$root" -l empty

cat > "$work/expected" <<'EOF_OWNER'
alpha 1.0
beta 2.0
EOF_OWNER
expect_success owner "$pkginfo" -r "$root" -o /usr/share/alpha/

status=0
"$pkginfo" -r "$root" -o /not/owned > "$work/actual" 2> "$work/stderr" || status=$?
test "$status" -eq 1 || fail "unowned query returned status $status"
test ! -s "$work/actual" || fail "unowned query wrote to stdout"
grep -Fx 'pkginfo: no package owns: /not/owned' "$work/stderr" > /dev/null ||
  fail "unowned query diagnostic differs"

status=0
"$pkginfo" -r "$root" -l absent > "$work/actual" 2> "$work/stderr" || status=$?
test "$status" -eq 1 || fail "missing list operand returned status $status"
test ! -s "$work/actual" || fail "missing list operand wrote to stdout"
grep -Fx \
  'pkginfo: argument is neither an installed package nor a package archive: absent' \
  "$work/stderr" > /dev/null || fail "missing list operand diagnostic differs"

status=0
"$pkginfo" -r "$root" -i -l alpha > "$work/actual" 2> "$work/stderr" || status=$?
test "$status" -eq 2 || fail "conflicting modes returned status $status"
test ! -s "$work/actual" || fail "conflicting modes wrote to stdout"
grep -F 'pkginfo: exactly one query mode is required' "$work/stderr" > /dev/null ||
  fail "conflicting mode diagnostic differs"

cat > "$work/expected" <<'EOF_VERSION'
pkginfo (libpkgstate) 0.2.0
EOF_VERSION
expect_success version "$pkginfo" --version
