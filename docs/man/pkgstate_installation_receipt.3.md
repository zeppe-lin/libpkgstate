% PKGSTATE_INSTALLATION_RECEIPT(3) libpkgstate | Version 3.0.0

<!-- Generated from pkgstate_installation_receipt.3.scdoc; do not edit. -->


# NAME

pkgstate_installation_receipt - complete native installation admission record

# SYNOPSIS

**#include <libpkgstate/installation_receipt.h>**

# DESCRIPTION

An **installation_receipt** binds:

- one complete **installed_control** value;
- one exact **state_target_binding**;
- the completed ownership manifest;
- the accepted operation-plan identity;
- the completed application-evidence identity; and
- optional composed-transaction evidence.

The receipt identity is computed from every field using the
**pkgstate/installation-receipt/1** domain. The constructor sorts and validates the
manifest and rejects duplicate paths. An empty manifest is valid for a package
that installs no filesystem objects.

Each owned entry retains complete recorded active-object metadata: object kind,
mode, uid, gid, modification time, kind-specific size, content, symbolic-link or
device data, and optional regular-file hard-link anchor. It also retains active
origin and optional rejected-object provenance.

# AUTHORITY

The receipt is not reconstructed from a package filename, archive metadata,
recipe directory, or target filesystem scan. Source, verified build, plan, artifact-content, and application identities are
supplied by their owning authorities and retained as typed references.

# PUBLICATION

Installation and replacement deltas carry a complete **installed_package** made
from this receipt. Removal names the expected installed package and carries no
new receipt.

# SEE ALSO

**pkgstate_model**(3), **libpkgstate-apply**(3),
**pkgstate_publication**(3)
