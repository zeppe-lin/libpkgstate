# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

libpkgstate_source = library(
  'pkgstate-source',
  'adapter.cpp',
  include_directories: public_inc,
  dependencies: [libpkgstate_dep, libpkgsource_dep],
  install: true,
  version: meson.project_version(),
  soversion: '1',
)

libpkgstate_source_dep = declare_dependency(
  link_with: libpkgstate_source,
  include_directories: public_inc,
  dependencies: [libpkgstate_dep, libpkgsource_dep],
)

meson.override_dependency('libpkgstate-source', libpkgstate_source_dep)

install_headers(
  '../include/libpkgstate-source/adapter.h',
  subdir: 'libpkgstate-source',
)

pkgconf.generate(
  libpkgstate_source,
  name: 'libpkgstate-source',
  filebase: 'libpkgstate-source',
  description: 'Projection from sealed package-source authority into libpkgstate',
  subdirs: '.',
  requires: [
    'libpkgstate >= 1.0.0',
    'libpkgsource >= 1.0.0',
  ],
)
