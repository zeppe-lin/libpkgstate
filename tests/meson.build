# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

if get_option('tests').allowed()
  sh = find_program('sh')
  model_test = executable(
    'model-test',
    'model_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('validate installed package state', model_test)

  legacy_text_store_test = executable(
    'legacy-text-store-test',
    'legacy_text_store_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('parse legacy package state', legacy_text_store_test)

  write_transaction_test = executable(
    'write-transaction-test',
    'write_transaction_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('commit legacy package state', write_transaction_test)

  locking_test = executable(
    'locking-test',
    'locking_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('interoperate through directory locks', locking_test)

  umbrella_header_test = executable(
    'umbrella-header-test',
    'umbrella_header_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('compile umbrella header', umbrella_header_test)

  public_headers = [
    ['libpkgstate', 'libpkgstate/libpkgstate.h'],
    ['error', 'libpkgstate/error.h'],
    ['package-identity', 'libpkgstate/package_identity.h'],
    ['owned-entry', 'libpkgstate/owned_entry.h'],
    ['installed-package', 'libpkgstate/installed_package.h'],
    ['snapshot', 'libpkgstate/snapshot.h'],
    ['write-transaction', 'libpkgstate/write_transaction.h'],
    ['store', 'libpkgstate/store.h'],
    ['legacy-text-store', 'libpkgstate/legacy_text_store.h'],
  ]

  foreach header : public_headers
    public_header_test = executable(
      header[0] + '-public-header-test',
      'public_header_test.cpp',
      cpp_args: [
        '-DLIBPKGSTATE_TEST_HEADER="' + header[1] + '"',
      ],
      dependencies: libpkgstate_dep,
    )
    test('compile public header ' + header[1], public_header_test)
  endforeach

  test(
    'verify documentation source contracts',
    sh,
    args: [
      join_paths(
        meson.current_source_dir(),
        'documentation_source_test.sh',
      ),
      meson.project_source_root(),
    ],
  )

  test(
    'verify pkg-config dependency metadata',
    sh,
    args: [
      join_paths(meson.current_source_dir(), 'pkgconfig_metadata_test.sh'),
      join_paths(
        meson.project_build_root(),
        'meson-private',
        'libpkgstate.pc',
      ),
    ],
  )

  if get_option('tools').allowed()
    tar_program = find_program('tar')

    test(
      'exercise pkginfo reference tool',
      sh,
      args: [
        join_paths(meson.current_source_dir(), 'pkginfo_cli_test.sh'),
        pkginfo_tool,
        tar_program,
      ],
    )

    test(
      'verify pkginfo manual contract',
      sh,
      args: [
        join_paths(meson.current_source_dir(), 'pkginfo_manual_test.sh'),
        meson.project_source_root(),
        pkginfo_tool,
      ],
    )
  endif
endif
