project(
  'libpkgstate',
  'cpp',
  version: '0.3.0',
  license: 'GPL-3.0-or-later',
  meson_version: '>=1.6.0',
  default_options: [
    'cpp_std=c++17',
    'warning_level=3',
    'default_library=shared',
    'b_ndebug=if-release',
  ],
)

library_kind = get_option('default_library')
if library_kind == 'both'
  error('libpkgstate requires separate shared and static builds')
endif

want_static = get_option('link_mode') == 'static'
if (library_kind == 'static') != want_static
  error('default_library and link_mode must both select shared or static')
endif

libpkgimage_dep = dependency(
  'libpkgimage',
  version: '>=0.3.0',
  required: get_option('tools'),
  static: want_static,
)

pkgconf = import('pkgconfig')

libcrypto_dep = dependency(
  'libcrypto',
  required: true,
  static: want_static,
)

libpkgplan_dep = dependency(
  'libpkgplan',
  version: '>=0.1.0',
  required: get_option('planner_adapter'),
  static: want_static,
  fallback: ['libpkgplan', 'libpkgplan_dep'],
  default_options: [
    'default_library=' + library_kind,
    'link_mode=' + get_option('link_mode'),
  ],
)

if get_option('install_tools') and get_option('tools').disabled()
  error('install_tools requires tools to be enabled')
endif

subdir('src')
if libpkgplan_dep.found()
  subdir('adapter')
endif
subdir('tools')
subdir('man')
subdir('tests')
