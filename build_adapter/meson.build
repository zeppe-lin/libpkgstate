# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

libpkgstate_build = library(
  'pkgstate-build',
  'adapter.cpp',
  include_directories: public_inc,
  dependencies: [
    libpkgstate_dep,
    libpkgstate_source_dep,
    libpkgbuild_dep,
    libpkgimage_dep,
  ],
  install: true,
  version: meson.project_version(),
  soversion: '1',
)

libpkgstate_build_dep = declare_dependency(
  link_with: libpkgstate_build,
  include_directories: public_inc,
  dependencies: [
    libpkgstate_dep,
    libpkgstate_source_dep,
    libpkgbuild_dep,
    libpkgimage_dep,
  ],
)

meson.override_dependency('libpkgstate-build', libpkgstate_build_dep)

install_headers(
  '../include/libpkgstate-build/adapter.h',
  subdir: 'libpkgstate-build',
)

pkgconf.generate(
  libpkgstate_build,
  name: 'libpkgstate-build',
  filebase: 'libpkgstate-build',
  description: 'Admission of verified libpkgbuild authority into libpkgstate',
  subdirs: '.',
  requires: [
    'libpkgstate >= 2.0.0',
    'libpkgstate-source >= 2.0.0',
    'libpkgbuild >= 1.0.0',
    'libpkgimage >= 0.3.0',
  ],
)
