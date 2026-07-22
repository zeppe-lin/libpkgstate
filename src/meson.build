# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

public_inc = include_directories('../include')

libpkgstate = library(
  'pkgstate',
  [
    'error.cpp',
    'digest.cpp',
    'canonical_record.cpp',
    'package_release.cpp',
    'installed_control.cpp',
    'state_target_binding.cpp',
    'package_identity.cpp',
    'package_path.cpp',
    'owned_entry.cpp',
    'installed_package.cpp',
    'snapshot.cpp',
    'publication_request.cpp',
    'legacy_installed_package.cpp',
    'legacy_snapshot.cpp',
    'legacy_text_store.cpp',
  ],
  cpp_args: ['-D_FILE_OFFSET_BITS=64'],
  include_directories: public_inc,
  dependencies: libcrypto_dep,
  install: true,
  version: meson.project_version(),
  soversion: '0',
)

libpkgstate_dep = declare_dependency(
  link_with: libpkgstate,
  include_directories: public_inc,
  dependencies: libcrypto_dep,
  compile_args: ['-D_FILE_OFFSET_BITS=64'],
)

meson.override_dependency('libpkgstate', libpkgstate_dep)

install_headers(
  '../include/libpkgstate/libpkgstate.h',
  '../include/libpkgstate/error.h',
  '../include/libpkgstate/digest.h',
  '../include/libpkgstate/package_release.h',
  '../include/libpkgstate/installed_control.h',
  '../include/libpkgstate/state_target_binding.h',
  '../include/libpkgstate/package_identity.h',
  '../include/libpkgstate/package_path.h',
  '../include/libpkgstate/owned_entry.h',
  '../include/libpkgstate/installed_package.h',
  '../include/libpkgstate/snapshot.h',
  '../include/libpkgstate/publication_request.h',
  '../include/libpkgstate/legacy_installed_package.h',
  '../include/libpkgstate/legacy_snapshot.h',
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
  requires_private: [libcrypto_dep],
)
