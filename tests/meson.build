# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

if get_option('tests').allowed()
  sh = find_program('sh')
  digest_test = executable(
    'digest-test',
    'digest_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('validate versioned state identities', digest_test)

  canonical_record_test = executable(
    'canonical-record-test',
    'canonical_record_test.cpp',
    include_directories: include_directories('../src'),
    dependencies: libpkgstate_dep,
  )
  test('hash canonical state records', canonical_record_test)

  package_release_test = executable(
    'package-release-test',
    'package_release_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('identify canonical package releases', package_release_test)

  installed_control_test = executable(
    'installed-control-test',
    'installed_control_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('retain durable installed control', installed_control_test)

  state_target_binding_test = executable(
    'state-target-binding-test',
    'state_target_binding_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('identify durable target-state bindings',
       state_target_binding_test)

  package_path_test = executable(
    'package-path-test',
    'package_path_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('normalize installed package paths', package_path_test)

  model_test = executable(
    'model-test',
    'model_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('validate installed package state', model_test)

  snapshot_identity_test = executable(
    'snapshot-identity-test',
    'snapshot_identity_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('identify ownership and installed snapshots',
       snapshot_identity_test)

  publication_request_test = executable(
    'publication-request-test',
    'publication_request_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('validate immutable state publication requests',
       publication_request_test)

  publication_receipt_test = executable(
    'publication-receipt-test',
    'publication_receipt_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('classify typed state publication receipts',
       publication_receipt_test)

  canonical_generation_store_test = executable(
    'canonical-generation-store-test',
    'canonical_generation_store_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('publish immutable canonical generations',
       canonical_generation_store_test)

  canonical_store_test = executable(
    'canonical-store-test',
    'canonical_store_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('enforce canonical compare and publish', canonical_store_test)

  legacy_model_test = executable(
    'legacy-model-test',
    'legacy_model_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('preserve incomplete legacy state', legacy_model_test)

  legacy_completeness_test = executable(
    'legacy-completeness-test',
    'legacy_completeness_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('identify incomplete legacy observations', legacy_completeness_test)

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
    ['digest', 'libpkgstate/digest.h'],
    ['package-release', 'libpkgstate/package_release.h'],
    ['installed-control', 'libpkgstate/installed_control.h'],
    ['state-target-binding',
     'libpkgstate/state_target_binding.h'],
    ['package-identity', 'libpkgstate/package_identity.h'],
    ['package-path', 'libpkgstate/package_path.h'],
    ['owned-entry', 'libpkgstate/owned_entry.h'],
    ['installed-package', 'libpkgstate/installed_package.h'],
    ['snapshot', 'libpkgstate/snapshot.h'],
    ['publication-request',
     'libpkgstate/publication_request.h'],
    ['publication-receipt',
     'libpkgstate/publication_receipt.h'],
    ['canonical-store', 'libpkgstate/canonical_store.h'],
    ['canonical-generation-store',
     'libpkgstate/canonical_generation_store.h'],
    ['legacy-completeness', 'libpkgstate/legacy_completeness.h'],
    ['legacy-installed-package',
     'libpkgstate/legacy_installed_package.h'],
    ['legacy-snapshot', 'libpkgstate/legacy_snapshot.h'],
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

  test(
    'verify release metadata',
    sh,
    args: [
      join_paths(meson.current_source_dir(), 'release_metadata_test.sh'),
      meson.project_source_root(),
    ],
  )

  if get_option('tools').allowed()
    package_path_conformance_test = executable(
      'package-path-conformance-test',
      'package_path_conformance_test.cpp',
      dependencies: [libpkgstate_dep, libpkgimage_dep],
    )
    test(
      'conform installed and archive package paths',
      package_path_conformance_test,
    )

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
