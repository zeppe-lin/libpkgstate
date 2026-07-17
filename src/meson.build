public_inc = include_directories('../include')

libpkgstate = library(
  'pkgstate',
  [
    'error.cpp',
    'package_identity.cpp',
    'owned_entry.cpp',
    'installed_package.cpp',
    'snapshot.cpp',
    'legacy_text_store.cpp',
  ],
  cpp_args: ['-D_FILE_OFFSET_BITS=64'],
  include_directories: public_inc,
  dependencies: libpkgimage_dep,
  install: true,
  version: meson.project_version(),
  soversion: '0',
)

libpkgstate_dep = declare_dependency(
  link_with: libpkgstate,
  include_directories: public_inc,
  dependencies: libpkgimage_dep,
  compile_args: ['-D_FILE_OFFSET_BITS=64'],
)

meson.override_dependency('libpkgstate', libpkgstate_dep)

install_headers(
  '../include/libpkgstate/libpkgstate.h',
  '../include/libpkgstate/error.h',
  '../include/libpkgstate/package_identity.h',
  '../include/libpkgstate/owned_entry.h',
  '../include/libpkgstate/installed_package.h',
  '../include/libpkgstate/snapshot.h',
  '../include/libpkgstate/write_transaction.h',
  '../include/libpkgstate/store.h',
  '../include/libpkgstate/legacy_text_store.h',
  subdir: 'libpkgstate',
)

pkgconf.generate(
  libpkgstate,
  name: 'libpkgstate',
  filebase: 'libpkgstate',
  description: 'Installed package state and ownership library',
  subdirs: '.',
  requires: ['libpkgimage >= 0.2.1'],
)
