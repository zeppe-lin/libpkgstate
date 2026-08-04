#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
[ "$#" -eq 1 ] || { echo "usage: lint-manpages.sh BUILD-DIR" >&2; exit 2; }
expected='libpkgstate.3
pkgstate_authority.7
pkgstate_model.3
pkgstate_installation_receipt.3
pkgstate_publication.3
pkgstate_store.3'
printf '%s\n' "$expected" | while IFS= read -r name; do
  page=$1/man/$name
  [ -s "$page" ] || { echo "generated manual is absent: $page" >&2; exit 1; }
  output=$(mandoc -Tlint "$page" 2>&1) || { printf '%s\n' "$output" >&2; exit 1; }
  [ -z "$output" ] || { printf '%s\n' "$output" >&2; exit 1; }
done
