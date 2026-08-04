# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

public_inc = include_directories('../include')

elf_export_generator = find_program(
  meson.project_source_root() / 'tools/generate-elf-export-script.sh',
)
core_export_script_name = 'libpkgstate.exports.map'
core_export_script_path = meson.current_build_dir() / core_export_script_name
core_export_script = configure_file(
  input: '../abi/libpkgstate.exports',
  output: core_export_script_name,
  command: [elf_export_generator, '@INPUT@'],
  capture: true,
)

libpkgstate = library(
  'pkgstate',
  [
    'error.cpp',
    'digest.cpp',
    'canonical_record.cpp',
    'model.cpp',
    'package_release.cpp',
    'package_source_record.cpp',
    'installed_control.cpp',
    'state_target_binding.cpp',
    'package_path.cpp',
    'owned_entry.cpp',
    'installation_receipt.cpp',
    'installed_package.cpp',
    'snapshot.cpp',
    'publication_request.cpp',
    'publication_projection.cpp',
    'publication_receipt.cpp',
    'publication_codec.cpp',
    'canonical_store.cpp',
  ],
  cpp_args: visibility_cpp_args + [
    '-DPKGSTATE_BUILDING_LIBRARY',
    '-D_FILE_OFFSET_BITS=64',
  ],
  gnu_symbol_visibility: 'hidden',
  link_args: ['-Wl,--version-script=' + core_export_script_path],
  link_depends: [core_export_script],
  include_directories: public_inc,
  dependencies: libcrypto_dep,
  install: true,
  version: meson.project_version(),
  soversion: '3',
)

libpkgstate_consumer_deps = []
if want_static
  libpkgstate_consumer_deps += [libcrypto_dep]
endif

libpkgstate_dep = declare_dependency(
  link_with: libpkgstate,
  include_directories: public_inc,
  dependencies: libpkgstate_consumer_deps,
  compile_args: ['-D_FILE_OFFSET_BITS=64'],
  version: meson.project_version(),
)

meson.override_dependency('libpkgstate', libpkgstate_dep)

install_headers(
  '../include/libpkgstate/libpkgstate.h',
  '../include/libpkgstate/export.h',
  '../include/libpkgstate/error.h',
  '../include/libpkgstate/digest.h',
  '../include/libpkgstate/model.h',
  '../include/libpkgstate/package_release.h',
  '../include/libpkgstate/package_source_record.h',
  '../include/libpkgstate/installed_control.h',
  '../include/libpkgstate/state_target_binding.h',
  '../include/libpkgstate/package_path.h',
  '../include/libpkgstate/owned_entry.h',
  '../include/libpkgstate/installation_receipt.h',
  '../include/libpkgstate/installed_package.h',
  '../include/libpkgstate/snapshot.h',
  '../include/libpkgstate/publication_request.h',
  '../include/libpkgstate/publication_receipt.h',
  '../include/libpkgstate/publication_codec.h',
  '../include/libpkgstate/canonical_store.h',
  subdir: 'libpkgstate',
)

pkgconf.generate(
  libpkgstate,
  name: 'libpkgstate',
  filebase: 'libpkgstate',
  description: 'Canonical installed package state and ownership authority',
  subdirs: '.',
  requires_private: [libcrypto_dep],
)
