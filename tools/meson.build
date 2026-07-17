if get_option('tools').allowed()
  pkginfo_tool = executable(
    'pkginfo',
    'pkginfo.cpp',
    cpp_args: [
      '-DPKGSTATE_VERSION="' + meson.project_version() + '"',
    ],
    dependencies: libpkgstate_dep,
    install: get_option('install_tools'),
  )
endif
