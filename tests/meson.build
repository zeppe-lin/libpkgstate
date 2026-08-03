# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
if get_option('tests').allowed()
  sh = find_program('sh')
  if library_kind == 'shared'
    nm = find_program('nm')
    test(
      'verify reviewed core ABI surface',
      find_program(meson.current_source_dir() / 'check_abi_surface.sh'),
      args: [
        libpkgstate.full_path(),
        meson.project_source_root() / 'abi/libpkgstate.exports',
        nm.full_path(),
      ],
    )
  endif
  core_tests = [
    ['digest', 'validate native identity domains'],
    ['canonical_record', 'hash canonical state records'],
    ['model', 'validate native installed-state values'],
    ['package_release', 'retain source-authoritative package releases'],
    ['package_source_record', 'seal durable package-source records'],
    ['installed_control', 'retain complete installed control'],
    ['state_target_binding', 'identify durable target-state bindings'],
    ['package_path', 'normalize installed package paths'],
    ['installation_receipt', 'seal native installation receipts'],
    ['snapshot_identity', 'identify ownership and installed snapshots'],
    ['publication_request', 'validate immutable publication requests'],
    ['publication_receipt', 'classify typed publication receipts'],
    ['publication_codec', 'reopen durable publication evidence'],
    ['canonical_generation_store', 'publish immutable canonical generations'],
    ['canonical_store', 'enforce canonical compare and publish'],
    ['umbrella_header', 'compile the native umbrella header'],
  ]
  foreach item : core_tests
    test_dependencies = [libpkgstate_dep]
    if item[0] == 'publication_codec'
      test_dependencies += libcrypto_dep
    endif
    test_sources = [item[0] + '_test.cpp']
    if item[0] == 'canonical_record'
      test_sources += '../src/canonical_record.cpp'
    endif
    target = executable(
      item[0] + '-test',
      test_sources,
      include_directories: include_directories('../src'),
      dependencies: test_dependencies,
    )
    test(item[1], target)
  endforeach

  public_headers = [
    ['libpkgstate', 'libpkgstate/libpkgstate.h'],
    ['export', 'libpkgstate/export.h'],
    ['error', 'libpkgstate/error.h'],
    ['digest', 'libpkgstate/digest.h'],
    ['model', 'libpkgstate/model.h'],
    ['package-release', 'libpkgstate/package_release.h'],
    ['package-source-record', 'libpkgstate/package_source_record.h'],
    ['installed-control', 'libpkgstate/installed_control.h'],
    ['state-target-binding', 'libpkgstate/state_target_binding.h'],
    ['package-path', 'libpkgstate/package_path.h'],
    ['owned-entry', 'libpkgstate/owned_entry.h'],
    ['installation-receipt', 'libpkgstate/installation_receipt.h'],
    ['installed-package', 'libpkgstate/installed_package.h'],
    ['snapshot', 'libpkgstate/snapshot.h'],
    ['publication-request', 'libpkgstate/publication_request.h'],
    ['publication-receipt', 'libpkgstate/publication_receipt.h'],
    ['publication-codec', 'libpkgstate/publication_codec.h'],
    ['canonical-store', 'libpkgstate/canonical_store.h'],
    ['canonical-generation-store', 'libpkgstate/canonical_generation_store.h'],
  ]
  foreach header : public_headers
    target = executable(
      header[0] + '-public-header-test',
      'public_header_test.cpp',
      cpp_args: ['-DLIBPKGSTATE_TEST_HEADER="' + header[1] + '"'],
      dependencies: libpkgstate_dep,
    )
    test('compile public header ' + header[1], target)
  endforeach

  foreach contract : [
    'publication_codec_source_test.sh',
    'documentation_source_test.sh',
    'ci_qualification_source_test.sh',
    'architecture_contract_test.sh',
    'repository_contract_test.sh',
  ]
    test(
      contract.replace('_test.sh', '').replace('_', '-'),
      sh,
      args: [join_paths(meson.current_source_dir(), contract), meson.project_source_root()],
    )
  endforeach
  test(
    'verify source style contract',
    sh,
    args: [
      join_paths(meson.current_source_dir(), 'style_contract_test.sh'),
      meson.project_source_root(),
      meson.project_build_root(),
    ],
  )
  test(
    'verify pkg-config dependency metadata',
    sh,
    args: [join_paths(meson.current_source_dir(), 'pkgconfig_metadata_test.sh'), join_paths(meson.project_build_root(), 'meson-private', 'libpkgstate.pc')],
  )
  test(
    'verify release metadata',
    sh,
    args: [join_paths(meson.current_source_dir(), 'release_metadata_test.sh'), meson.project_source_root()],
  )
  if get_option('tools').allowed()
    pkgstate_check_fixture = executable('pkgstate-check-fixture', 'pkgstate_check_fixture.cpp', dependencies: libpkgstate_dep)
    test('exercise read-only native state diagnostics', sh, args: [join_paths(meson.current_source_dir(), 'pkgstate_check_cli_test.sh'), pkgstate_check_tool, pkgstate_check_fixture])
    test('verify pkgstate-check manual contract', sh, args: [join_paths(meson.current_source_dir(), 'pkgstate_check_manual_test.sh'), meson.project_source_root(), pkgstate_check_tool])
  endif
endif
