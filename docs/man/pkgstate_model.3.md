% PKGSTATE_MODEL(3) libpkgstate | Version 3.0.0

<!-- Generated from pkgstate_model.3.scdoc; do not edit. -->


# NAME

pkgstate_model - native installed-state value model

# SOURCE RECORD

**package_source_record** retains:

- the source-owned **package_release_identity** and release coordinates;
- package metadata needed after source discovery;
- typed runtime package requirements with declaration and profile-expansion
  provenance;
- all lifecycle programs and action-bound lifecycle requirements;
- declared and selected build and target architectures;
- selected build profiles with source-owned identities and declarations;
- source recipe identity; and
- source snapshot identity.

The record is normalized and immutable. Its identity is owned by libpkgstate and
binds every retained field.

# INSTALLED CONTROL

**installed_control** contains one source record, one **installation_reason**, and
one **build_provenance** value.

Installation reason is exactly one of:

**explicit_request**
The package was directly requested.

**runtime_dependency**
Another exact package caused installation.

**profile_membership**
A named source profile and its source-owned identity caused installation.

**system_policy**
A named system policy caused installation.

Build provenance retains typed identities for the exact source record, build
request, verified source-material set, materialized build-input set, environment
policy, build policy, successful build result, payload manifest, sealed artifact,
exact artifact content, artifact binding, execution evidence, normalized artifact
image, and image-inspection receipt.

# OWNERSHIP

**owned_entry** records one normalized package path, complete active-object metadata,
active origin, and an optional rejected-object reference. Object kinds include
regular file, directory, symbolic link, FIFO, character device, block device,
socket, and other. Metadata retains mode, uid, gid, modification time, required
kind-specific facts, and an optional regular-file hard-link anchor. Active origin
distinguishes incoming payload from a retained existing object. A rejected
reference identifies whether the incoming or prior installed object was staged
aside.

# INSTALLED PACKAGE

**installation_receipt** is complete admission evidence. **installed_package** is a
thin immutable authority value constructed only from that receipt. Its identity
binds the receipt and target.

# SNAPSHOT

**snapshot** contains all installed packages for one **state_target_binding**. It
normalizes package ordering, rejects duplicate package names, validates every
package target, derives path ownership, and computes independent ownership and
snapshot identities.

A pathname is a locator, not a target identity. The target binding separately
names the managed target, state store, root view, state backend, and publication
domain.

# INVARIANTS

- Package, profile, and architecture references are normalized identifiers.
- Every installed package has complete source, verified build, target, application,
  and ownership evidence.
- Build provenance names the same exact source record retained by installed control.
- Runtime and lifecycle requirements are typed exact package references.
- Lifecycle requirements are bound to one lifecycle action.
- Package release identity remains source-owned.
- State-owned identities cannot be supplied by callers.
- Duplicate packages, paths, profile selections, requirements, or lifecycle
  actions are rejected where their model requires uniqueness.

# SEE ALSO

**pkgstate_installation_receipt**(3), **pkgstate_publication**(3),
**pkgstate-generation**(5)
