# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

plan_adapter_inc = include_directories('../include')

libpkgstate_plan = library(
  'pkgstate-plan',
  'plan_adapter.cpp',
  cpp_args: ['-D_FILE_OFFSET_BITS=64'],
  include_directories: plan_adapter_inc,
  dependencies: [libpkgstate_dep, libpkgplan_dep],
  install: true,
  version: meson.project_version(),
  soversion: '0',
)

libpkgstate_plan_dep = declare_dependency(
  link_with: libpkgstate_plan,
  include_directories: plan_adapter_inc,
  dependencies: [libpkgstate_dep, libpkgplan_dep],
  compile_args: ['-D_FILE_OFFSET_BITS=64'],
)

meson.override_dependency('libpkgstate-plan', libpkgstate_plan_dep)

install_headers(
  '../include/libpkgstate-plan/adapter.h',
  subdir: 'libpkgstate-plan',
)

pkgconf.generate(
  libpkgstate_plan,
  name: 'libpkgstate-plan',
  filebase: 'libpkgstate-plan',
  description: 'Explicit libpkgstate to libpkgplan projection adapter',
  subdirs: '.',
  requires: [
    'libpkgstate = ' + meson.project_version(),
    'libpkgplan >= 0.1.0',
  ],
)
