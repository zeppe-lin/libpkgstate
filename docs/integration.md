# Integration

Consumers construct durable state through explicit adapters and publish through a selected `canonical_store` provider.

```text
libpkgsource -> libpkgstate-source -> package_source_record
                                      |
libpkgbuild + libpkgimage -> libpkgstate-build -> build authority

libpkgstate -> libpkgstate-plan -> planner facts

libpkgapply + source/build admission -> libpkgstate-apply
                                      -> state_publication_request
                                      -> libpkgstate canonical_store
                                      -> selected provider

libpkgstate generation codec -> canonical binding and snapshot bytes
libpkgstate-posix             -> generation-v3 filesystem storage
```

A consumer links only the bridges and storage provider it uses. Linking `libpkgstate` alone exposes the pure generation record protocol but never pulls a source, build, image, plan, apply, or filesystem mechanism authority into the process.

When an adjacent owner changes, inspect its exact public body and qualify the corresponding adapter repository. Do not compensate by adding compatibility or reacquisition logic to the state core.
