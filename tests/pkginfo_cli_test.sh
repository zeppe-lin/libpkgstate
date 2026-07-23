#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu

case $1 in
  /*) pkginfo=$1 ;;
  *) pkginfo=$PWD/$1 ;;
esac
tar=$2
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

expect_success_from()
{
  name=$1
  directory=$2
  shift 2
  if ! (cd "$directory" && "$@") > "$work/actual" 2> "$work/stderr"; then
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

payload=$work/archive-root
mkdir -p \
  "$payload/etc" \
  "$payload/usr/bin" \
  "$payload/usr/share/empty"
printf 'archive configuration\n' > "$payload/etc/archive.conf"
printf '#!/bin/sh\nexit 0\n' > "$payload/usr/bin/archive-tool"
ln -s etc/archive.conf "$payload/archive-link"

archive=$work/example.pkg.tar
"$tar" -cf "$archive" -C "$payload" \
  etc/archive.conf \
  usr/share/empty \
  usr/bin/archive-tool \
  archive-link

cat > "$work/expected" <<'EOF_ARCHIVE'
etc/archive.conf
usr/share/empty/
usr/bin/archive-tool
archive-link
EOF_ARCHIVE
expect_success archive-list "$pkginfo" -r "$root" -l "$archive"

cp "$archive" "$work/alpha"
cat > "$work/expected" <<'EOF_PRECEDENCE'
etc/alpha.conf
usr/share/alpha/
EOF_PRECEDENCE
expect_success_from installed-precedence "$work" \
  "$pkginfo" -r "$root" -l alpha

bad_archive=$work/truncated.pkg.tar
dd if="$archive" of="$bad_archive" bs=100 count=1 2>/dev/null
status=0
"$pkginfo" -r "$root" -l "$bad_archive" \
  > "$work/actual" 2> "$work/stderr" || status=$?
test "$status" -eq 1 || fail "malformed archive returned status $status"
test ! -s "$work/actual" || fail "malformed archive wrote to stdout"
grep -F 'pkginfo:' "$work/stderr" > /dev/null ||
  fail "malformed archive diagnostic lacks tool prefix"
grep -F "$bad_archive" "$work/stderr" > /dev/null ||
  fail "malformed archive diagnostic omits pathname"

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

mkdir "$work/not-an-archive"
status=0
"$pkginfo" -r "$root" -l "$work/not-an-archive" \
  > "$work/actual" 2> "$work/stderr" || status=$?
test "$status" -eq 1 || fail "directory list operand returned status $status"
test ! -s "$work/actual" || fail "directory list operand wrote to stdout"
expected_error='pkginfo: argument is neither an installed package nor a package archive'
expected_error="$expected_error: $work/not-an-archive"
grep -Fx "$expected_error" "$work/stderr" > /dev/null ||
  fail "directory list operand diagnostic differs"

status=0
"$pkginfo" -r "$root" -i -l alpha > "$work/actual" 2> "$work/stderr" || status=$?
test "$status" -eq 2 || fail "conflicting modes returned status $status"
test ! -s "$work/actual" || fail "conflicting modes wrote to stdout"
grep -F 'pkginfo: exactly one query mode is required' "$work/stderr" > /dev/null ||
  fail "conflicting mode diagnostic differs"

cat > "$work/expected" <<'EOF_VERSION'
pkginfo (libpkgstate) 0.4.0
EOF_VERSION
expect_success version "$pkginfo" --version
