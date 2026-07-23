#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

set -eu

if [ "$#" -ne 1 ]; then
  echo "usage: lint-manpages.sh BUILD-DIR" >&2
  exit 2
fi

build_dir=$1
expected='libpkgstate.3
pkgstate_authority.7
pkgstate_model.3
pkgstate_publication.3
pkgstate_store.3
pkgstate_canonical_generation_store.3
pkgstate_legacy_compatibility.3
pkgstate_legacy_import.3
pkgstate_plan_adapter.3
pkgstate_legacy_text_store.3
pkgstate-db.5
pkgstate-generation.5
pkginfo.1
pkgstate-check.1'

printf '%s\n' "$expected" |
  while IFS= read -r name; do
    page=$build_dir/man/$name
    [ -s "$page" ] || {
      echo "generated manual is absent: $page" >&2
      exit 1
    }
    output=$(mandoc -Tlint "$page" 2>&1) || {
      printf '%s\n' "$output" >&2
      exit 1
    }
    [ -z "$output" ] || {
      printf '%s\n' "$output" >&2
      exit 1
    }
  done
