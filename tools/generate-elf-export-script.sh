#!/bin/sh
# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu

if [ "$#" -ne 1 ]; then
  echo "usage: $0 MANIFEST" >&2
  exit 2
fi

manifest=$1

# The reviewed manifest is a byte-ordered protocol artifact. Configure-time
# generation must not depend on the maintainer's locale.
LC_ALL=C
export LC_ALL

awk '
function fail(message)
{
  print "generate-elf-export-script: " message > "/dev/stderr"
  exit 1
}

BEGIN {
  print "{"
  print "  global:"
}

/^[[:space:]]*($|#)/ {
  next
}

{
  if ($0 !~ /^_Z[A-Za-z0-9_]+$/) {
    fail("invalid symbol on line " NR ": " $0)
  }
  if ($0 !~ /8pkgstate/) {
    fail("foreign implementation export on line " NR ": " $0)
  }
  if (previous != "" && $0 <= previous) {
    fail("manifest is not uniquely C-locale sorted on line " NR ": " $0)
  }
  previous = $0
  print "    " $0 ";"
  count++
}

END {
  if (count == 0) {
    fail("manifest contains no symbols")
  }
  print "  local:"
  print "    *;"
  print "};"
}
' "$manifest"
