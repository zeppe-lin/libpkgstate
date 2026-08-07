# Integration

Consumers construct durable state through explicit adapters and publish through a selected `canonical_store` provider.

```text
libpkgsource -> libpkgstate-source -> package_source_record
                                      |
libpkgbuild-image -> libpkgstate-build -> durable build provenance

libpkgstate -> libpkgstate-plan -> planner facts

libpkgapply + source/build admission -> libpkgstate-apply
                                      -> state_publication_request
                                      -> libpkgstate canonical_store
                                      -> selected provider

libpkgstate generation codec -> canonical binding and snapshot bytes
libpkgstate-posix             -> generation-v1 filesystem storage
```

A consumer links only the bridges and storage provider it uses. Linking `libpkgstate` alone exposes the pure generation record protocol but never pulls a source, build, image, plan, apply, or filesystem mechanism authority into the process.

When an adjacent owner changes, inspect its exact public body and qualify the corresponding adapter repository. Do not compensate by adding compatibility or reacquisition logic to the state core.

## Pure publication projection

Controllers and evidence decoders that already hold one exact `state_publication_request` and its complete actual-prior `snapshot` call `project_publication_request()`. They must not copy package-delta application rules or reopen a state store merely to derive the resulting immutable state. The projection validates target, prior epoch, and every delta, and performs no storage access or target mutation.
