#!/bin/sh
set -eu
root=$1
for f in .github/workflows/ci.yml ci/configure-and-test.sh ci/qualify-installed.sh ci/lint-manpages.sh ci/installed-core-consumer.cpp ci/audit-shared-boundary.sh; do test -s "$root/$f" || exit 1; done
for s in "$root"/ci/*.sh; do sh -n "$s"; done
for v in 'GCC shared' 'GCC static' 'Clang shared' 'Clang static' 'GCC release' 'address,undefined' 'meson==1.10.2' '--wrap-mode=nofallback'; do grep -F -- "$v" "$root/.github/workflows/ci.yml" "$root/ci/configure-and-test.sh" >/dev/null || { echo "missing CI contract: $v" >&2; exit 1; }; done

test -x "$root/ci/audit-shared-boundary.sh" || { echo 'missing executable dynamic-boundary audit' >&2; exit 1; }

grep -F 'pkgstate_generation_codec.3' "$root/ci/lint-manpages.sh" >/dev/null || {
  echo 'generation codec manual is absent from CI qualification' >&2
  exit 1
}
