#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu

pkgstate_check=$1
fixture=$2

temporary=$(mktemp -d)

cleanup()
{
  chmod -R u+w "$temporary" 2>/dev/null || :
  rm -rf "$temporary"
}

trap cleanup EXIT HUP INT TERM

fail()
{
  echo "pkgstate-check-cli-test: $*" >&2
  exit 1
}

identity()
{
  digit=$1
  printf 'v1:sha256:'
  printf '%064d' 0 | tr 0 "$digit"
}

canonical=$temporary/canonical
legacy=$temporary/db
output=$temporary/output
error=$temporary/error
before=$temporary/before
after=$temporary/after

"$fixture" "$canonical"

snapshot_files()
{
  root=$1
  find "$root" -type f -print | LC_ALL=C sort |
    while IFS= read -r file
    do
      relative=${file#"$root"/}
      printf '%s ' "$relative"
      cksum < "$file"
    done
}

snapshot_files "$canonical" > "$before"

"$pkgstate_check" \
  --canonical-store "$canonical" \
  --managed-target "$(identity 1)" \
  --state-store "$(identity 2)" \
  --root-view "$(identity 3)" \
  --state-backend "$(identity 4)" \
  --publication-domain "$(identity 5)" \
  > "$output" 2> "$error" || fail 'canonical diagnostics failed'

test ! -s "$error" || fail 'canonical diagnostics wrote standard error'
grep -F 'mode=canonical' "$output" >/dev/null ||
  fail 'canonical output omits mode'
grep -F 'status=valid' "$output" >/dev/null ||
  fail 'canonical output omits validity'
grep -F 'storage-format=libpkgstate-generation-v1' "$output" >/dev/null ||
  fail 'canonical output omits storage format'
grep -E '^target-binding=v1:sha256:[0-9a-f]{64}$' "$output" >/dev/null ||
  fail 'canonical output omits target binding'
grep -E '^snapshot=v1:sha256:[0-9a-f]{64}$' "$output" >/dev/null ||
  fail 'canonical output omits snapshot identity'
grep -E '^ownership-inventory=v1:sha256:[0-9a-f]{64}$' "$output" >/dev/null ||
  fail 'canonical output omits ownership identity'
grep -F 'packages=1' "$output" >/dev/null ||
  fail 'canonical package count is wrong'
grep -F 'ownership-claims=2' "$output" >/dev/null ||
  fail 'canonical ownership claim count is wrong'
grep -F 'owned-paths=2' "$output" >/dev/null ||
  fail 'canonical owned-path count is wrong'
grep -F 'shared-paths=0' "$output" >/dev/null ||
  fail 'canonical shared-path count is wrong'
for line in \
  'control.recorded-at-installation=1' \
  'control.recorded-in-compatibility-storage=1' \
  'control.supplied-by-migration=1' \
  'control.historically-unavailable=1'
do
  grep -F "$line" "$output" >/dev/null ||
    fail "canonical output omits $line"
done

snapshot_files "$canonical" > "$after"
cmp -s "$before" "$after" ||
  fail 'canonical diagnostics changed durable file contents'

if "$pkgstate_check" \
  --canonical-store "$canonical" \
  --managed-target "$(identity 6)" \
  --state-store "$(identity 2)" \
  --root-view "$(identity 3)" \
  --state-backend "$(identity 4)" \
  --publication-domain "$(identity 5)" \
  > "$output" 2> "$error"
then
  fail 'mismatched canonical binding succeeded'
fi
test ! -s "$output" || fail 'failed canonical check wrote standard output'
grep -F 'does not match caller' "$error" >/dev/null ||
  fail 'mismatched binding diagnostic is missing'

missing=$temporary/missing
if "$pkgstate_check" \
  --canonical-store "$missing" \
  --managed-target "$(identity 1)" \
  --state-store "$(identity 2)" \
  --root-view "$(identity 3)" \
  --state-backend "$(identity 4)" \
  --publication-domain "$(identity 5)" \
  > "$output" 2> "$error"
then
  fail 'absent canonical store succeeded'
fi
test ! -e "$missing" || fail 'diagnostics initialized an absent store'

incomplete=$temporary/incomplete
mkdir "$incomplete"
printf 'unchanged\n' > "$incomplete/marker"
if "$pkgstate_check" \
  --canonical-store "$incomplete" \
  --managed-target "$(identity 1)" \
  --state-store "$(identity 2)" \
  --root-view "$(identity 3)" \
  --state-backend "$(identity 4)" \
  --publication-domain "$(identity 5)" \
  > "$output" 2> "$error"
then
  fail 'incomplete canonical store succeeded'
fi
test "$(cat "$incomplete/marker")" = unchanged ||
  fail 'incomplete-store marker changed'
test ! -e "$incomplete/binding" ||
  fail 'diagnostics published a binding file'
test ! -e "$incomplete/current" ||
  fail 'diagnostics published a selector'
test ! -e "$incomplete/generations" ||
  fail 'diagnostics created a generations directory'

cat > "$legacy" <<'DATABASE'
alpha
1-1
usr/bin/shared

beta
2-1
usr/bin/shared
usr/share/

DATABASE
cksum < "$legacy" > "$before"

"$pkgstate_check" --legacy-database "$legacy" \
  > "$output" 2> "$error" || fail 'legacy diagnostics failed'
test ! -s "$error" || fail 'legacy diagnostics wrote standard error'
grep -F 'mode=legacy' "$output" >/dev/null ||
  fail 'legacy output omits mode'
grep -F 'status=valid' "$output" >/dev/null ||
  fail 'legacy output omits validity'
grep -E '^snapshot-observation=v1:sha256:[0-9a-f]{64}$' \
  "$output" >/dev/null || fail 'legacy output omits observation identity'
grep -F 'packages=2' "$output" >/dev/null ||
  fail 'legacy package count is wrong'
grep -F 'ownership-claims=3' "$output" >/dev/null ||
  fail 'legacy ownership claim count is wrong'
grep -F 'owned-paths=2' "$output" >/dev/null ||
  fail 'legacy owned-path count is wrong'
grep -F 'shared-paths=1' "$output" >/dev/null ||
  fail 'legacy shared-path count is wrong'
grep -F \
  'package-fact.package-release=historically-unavailable' \
  "$output" >/dev/null || fail 'legacy package absence is hidden'
grep -F \
  'snapshot-fact.ownership-relation=derived-from-retained-facts' \
  "$output" >/dev/null || fail 'legacy derived ownership is hidden'
grep -F \
  'snapshot-fact.installed-state-snapshot-identity=historically-unavailable' \
  "$output" >/dev/null || fail 'legacy snapshot absence is hidden'
cksum < "$legacy" > "$after"
cmp -s "$before" "$after" || fail 'legacy diagnostics changed the database'

if "$pkgstate_check" --legacy-database "$legacy" \
  --managed-target "$(identity 1)" > "$output" 2> "$error"
then
  fail 'legacy mode accepted canonical binding input'
else
  status=$?
fi
test "$status" -eq 2 || fail 'legacy binding misuse did not return status 2'

if "$pkgstate_check" > "$output" 2> "$error"
then
  fail 'missing mode succeeded'
else
  status=$?
fi
test "$status" -eq 2 || fail 'missing mode did not return status 2'

"$pkgstate_check" --help > "$output" 2> "$error" ||
  fail 'help failed'
test ! -s "$error" || fail 'help wrote standard error'
grep -F 'never initializes a canonical store' "$output" >/dev/null ||
  fail 'help omits non-mutating boundary'

"$pkgstate_check" --version > "$output" 2> "$error" ||
  fail 'version failed'
test ! -s "$error" || fail 'version wrote standard error'
grep -E '^pkgstate-check \(libpkgstate\) [^[:space:]]+$' \
  "$output" >/dev/null || fail 'version output is malformed'
