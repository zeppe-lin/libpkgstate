# Integration

Consumers construct durable state through explicit adapters and publish through `canonical_store`.

```text
libpkgsource -> libpkgstate-source -> package_source_record
                                      |
libpkgbuild + libpkgimage -> libpkgstate-build -> build authority

libpkgstate -> libpkgstate-plan -> planner facts

libpkgapply + source/build admission -> libpkgstate-apply
                                      -> state_publication_request
                                      -> libpkgstate canonical_store
```

A consumer links only the bridges it uses. Linking `libpkgstate` alone never pulls a source, build, image, plan, or apply authority into the process.

When an adjacent owner changes, inspect its exact public body and qualify the corresponding adapter repository. Do not compensate by adding compatibility or reacquisition logic to the state core.
