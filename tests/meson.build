# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later
if get_option('tests').allowed()
  sh = find_program('sh')

  if library_kind == 'shared'
    nm = find_program('nm')
    test(
      'verify reviewed core ABI surface',
      find_program(meson.current_source_dir() / 'contracts/check_abi_surface.sh'),
      args: [
        libpkgstate.full_path(),
        meson.project_source_root() / 'abi/libpkgstate.exports',
        nm.full_path(),
      ],
      suite: 'contract',
    )
  endif

  unit_tests = [
    ['error', 'anchor public failure domains'],
    ['digest', 'validate native identity domains'],
    ['reference-provenance', 'validate references and declaration provenance'],
    ['source-control-model', 'validate source control model values'],
    ['installation-reason', 'validate installation reason shapes'],
    ['package-release', 'retain source-authoritative package releases'],
    ['state-target-binding', 'identify durable target-state bindings'],
    ['package-path', 'normalize installed package paths'],
    ['owned-entry', 'validate installed object and ownership values'],
  ]
  foreach item : unit_tests
    target = executable(
      item[0] + '-test',
      'unit/' + item[0].replace('-', '_') + '_test.cpp',
      dependencies: libpkgstate_dep,
    )
    test(item[1], target, suite: 'unit')
  endforeach

  integration_tests = [
    ['package-source-record', 'seal durable package-source records'],
    ['installed-control', 'retain complete installed control'],
    ['installation-receipt', 'seal native installation receipts'],
    ['installed-package', 'identify complete installed packages'],
    ['snapshot', 'identify ownership and installed snapshots'],
    ['publication-request', 'validate immutable publication requests'],
    ['publication-projection', 'project exact publication state without I/O'],
    ['publication-receipt', 'classify typed publication receipts'],
    ['canonical-store', 'enforce canonical compare and publish'],
  ]
  foreach item : integration_tests
    target = executable(
      item[0] + '-test',
      'integration/' + item[0].replace('-', '_') + '_test.cpp',
      dependencies: libpkgstate_dep,
    )
    test(item[1], target, suite: 'integration')
  endforeach

  protocol_tests = [
    ['canonical-record', 'hash canonical state records'],
    ['generation-codec', 'reopen canonical generation records'],
    ['publication-request-codec', 'reopen publication requests'],
    ['publication-receipt-codec', 'reopen publication receipts'],
  ]
  foreach item : protocol_tests
    sources = ['protocol/' + item[0].replace('-', '_') + '_test.cpp']
    dependencies = [libpkgstate_dep]
    if item[0] == 'canonical-record'
      sources += '../src/canonical_record.cpp'
      dependencies += libcrypto_dep
    elif item[0] in ['publication-request-codec', 'publication-receipt-codec']
      dependencies += libcrypto_dep
    endif
    target = executable(
      item[0] + '-test',
      sources,
      include_directories: include_directories('../src'),
      dependencies: dependencies,
    )
    test(item[1], target, suite: 'protocol')
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
    ['publication-projection', 'libpkgstate/publication_projection.h'],
    ['publication-receipt', 'libpkgstate/publication_receipt.h'],
    ['publication-codec', 'libpkgstate/publication_codec.h'],
    ['generation-codec', 'libpkgstate/generation_codec.h'],
    ['canonical-store', 'libpkgstate/canonical_store.h'],
  ]
  foreach header : public_headers
    target = executable(
      header[0] + '-public-header-test',
      'header/public_header_test.cpp',
      cpp_args: ['-DLIBPKGSTATE_TEST_HEADER="' + header[1] + '"'],
      dependencies: libpkgstate_dep,
    )
    test('compile public header ' + header[1], target, suite: 'header')
  endforeach

  target = executable(
    'umbrella-header-test',
    'header/umbrella_header_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('compile the native umbrella header', target, suite: 'header')

  foreach contract : [
    'abi_manifest_contract_test.sh',
    'publication_codec_source_test.sh',
    'documentation_source_test.sh',
    'ci_qualification_source_test.sh',
    'architecture_contract_test.sh',
    'repository_contract_test.sh',
    'check_test_layout.sh',
  ]
    test(
      contract.replace('_test.sh', '').replace('.sh', '').replace('_', '-'),
      sh,
      args: [join_paths(meson.current_source_dir(), 'contracts', contract), meson.project_source_root()],
      suite: 'contract',
    )
  endforeach
  test(
    'verify source style contract',
    sh,
    args: [
      join_paths(meson.current_source_dir(), 'contracts/style_contract_test.sh'),
      meson.project_source_root(),
      meson.project_build_root(),
    ],
    suite: 'contract',
  )
  test(
    'verify pkg-config dependency metadata',
    sh,
    args: [join_paths(meson.current_source_dir(), 'contracts/pkgconfig_metadata_test.sh'), join_paths(meson.project_build_root(), 'meson-private', 'libpkgstate.pc')],
    suite: 'contract',
  )
  test(
    'verify release metadata',
    sh,
    args: [join_paths(meson.current_source_dir(), 'contracts/release_metadata_test.sh'), meson.project_source_root()],
    suite: 'contract',
  )
endif
