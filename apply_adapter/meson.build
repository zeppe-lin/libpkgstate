# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

apply_adapter_inc = include_directories('../include')

libpkgstate_apply = library(
  'pkgstate-apply',
  'apply_adapter.cpp',
  cpp_args: ['-D_FILE_OFFSET_BITS=64'],
  include_directories: apply_adapter_inc,
  dependencies: [
    libpkgstate_dep,
    libpkgstate_source_dep,
    libpkgstate_build_dep,
    libpkgapply_dep,
  ],
  install: true,
  version: meson.project_version(),
  soversion: '3',
)

libpkgstate_apply_dep = declare_dependency(
  link_with: libpkgstate_apply,
  include_directories: apply_adapter_inc,
  dependencies: [
    libpkgstate_dep,
    libpkgstate_source_dep,
    libpkgstate_build_dep,
    libpkgapply_dep,
  ],
  compile_args: ['-D_FILE_OFFSET_BITS=64'],
)

meson.override_dependency('libpkgstate-apply', libpkgstate_apply_dep)

install_headers(
  '../include/libpkgstate-apply/adapter.h',
  subdir: 'libpkgstate-apply',
)

pkgconf.generate(
  libpkgstate_apply,
  name: 'libpkgstate-apply',
  filebase: 'libpkgstate-apply',
  description: 'Completed application to installed-state projection adapter',
  subdirs: '.',
  requires: [
    'libpkgstate >= 2.0.0',
    'libpkgstate-source >= 2.3.0',
    'libpkgstate-build >= 2.3.0',
    'libpkgapply >= 2.0.0',
  ],
)
