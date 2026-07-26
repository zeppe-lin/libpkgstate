# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

if get_option('tests').allowed()
  sh = find_program('sh')

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
    ['canonical_generation_store', 'publish immutable canonical generations'],
    ['canonical_store', 'enforce canonical compare and publish'],
    ['umbrella_header', 'compile the native umbrella header'],
  ]

  foreach item : core_tests
    target = executable(
      item[0] + '-test',
      item[0] + '_test.cpp',
      include_directories: include_directories('../src'),
      dependencies: libpkgstate_dep,
    )
    test(item[1], target)
  endforeach

  if libpkgsource_dep.found()
    source_adapter_test = executable(
      'source-adapter-test',
      'source_adapter_test.cpp',
      dependencies: libpkgstate_source_dep,
    )
    test('retain sealed libpkgsource authority', source_adapter_test)

    source_adapter_header_test = executable(
      'source-adapter-public-header-test',
      'public_header_test.cpp',
      cpp_args: ['-DLIBPKGSTATE_TEST_HEADER="libpkgstate-source/adapter.h"'],
      dependencies: libpkgstate_source_dep,
    )
    test('compile public header libpkgstate-source/adapter.h',
         source_adapter_header_test)

    test(
      'verify source adapter pkg-config metadata',
      sh,
      args: [
        join_paths(meson.current_source_dir(),
                   'source_adapter_metadata_test.sh'),
        meson.project_build_root(),
      ],
    )
  endif

  if libpkgbuild_dep.found() and libpkgimage_dep.found()
    build_adapter_test = executable(
      'build-adapter-test',
      'build_adapter_test.cpp',
      dependencies: libpkgstate_build_dep,
    )
    test('retain verified libpkgbuild authority', build_adapter_test)

    build_adapter_header_test = executable(
      'build-adapter-public-header-test',
      'public_header_test.cpp',
      cpp_args: ['-DLIBPKGSTATE_TEST_HEADER="libpkgstate-build/adapter.h"'],
      dependencies: libpkgstate_build_dep,
    )
    test('compile public header libpkgstate-build/adapter.h',
         build_adapter_header_test)

    test(
      'verify build adapter pkg-config metadata',
      sh,
      args: [
        join_paths(meson.current_source_dir(),
                   'build_adapter_metadata_test.sh'),
        meson.project_build_root(),
      ],
    )
  endif

  if libpkgplan_dep.found()
    plan_adapter_test = executable(
      'plan-adapter-test',
      'plan_adapter_test.cpp',
      dependencies: libpkgstate_plan_dep,
    )
    test('project native installed state into libpkgplan', plan_adapter_test)

    plan_adapter_header_test = executable(
      'plan-adapter-public-header-test',
      'public_header_test.cpp',
      cpp_args: ['-DLIBPKGSTATE_TEST_HEADER="libpkgstate-plan/adapter.h"'],
      dependencies: libpkgstate_plan_dep,
    )
    test('compile public header libpkgstate-plan/adapter.h',
         plan_adapter_header_test)

    test(
      'verify planner adapter pkg-config metadata',
      sh,
      args: [
        join_paths(meson.current_source_dir(),
                   'plan_adapter_metadata_test.sh'),
        meson.project_build_root(),
      ],
    )
  endif

  if libpkgapply_dep.found()
    apply_adapter_test = executable(
      'apply-adapter-test',
      'apply_adapter_test.cpp',
      dependencies: libpkgstate_apply_dep,
    )
    test('admit completed application as native state', apply_adapter_test)

    apply_adapter_header_test = executable(
      'apply-adapter-public-header-test',
      'public_header_test.cpp',
      cpp_args: ['-DLIBPKGSTATE_TEST_HEADER="libpkgstate-apply/adapter.h"'],
      dependencies: libpkgstate_apply_dep,
    )
    test('compile public header libpkgstate-apply/adapter.h',
         apply_adapter_header_test)

    test(
      'verify application adapter pkg-config metadata',
      sh,
      args: [
        join_paths(meson.current_source_dir(),
                   'apply_adapter_metadata_test.sh'),
        meson.project_build_root(),
      ],
    )
  endif

  public_headers = [
    ['libpkgstate', 'libpkgstate/libpkgstate.h'],
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
    ['canonical-store', 'libpkgstate/canonical_store.h'],
    ['canonical-generation-store',
     'libpkgstate/canonical_generation_store.h'],
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

  test(
    'verify documentation source contracts',
    sh,
    args: [join_paths(meson.current_source_dir(),
                      'documentation_source_test.sh'),
           meson.project_source_root()],
  )
  test(
    'verify CI qualification source contracts',
    sh,
    args: [join_paths(meson.current_source_dir(),
                      'ci_qualification_source_test.sh'),
           meson.project_source_root()],
  )
  test(
    'verify pkg-config dependency metadata',
    sh,
    args: [join_paths(meson.current_source_dir(),
                      'pkgconfig_metadata_test.sh'),
           join_paths(meson.project_build_root(), 'meson-private',
                      'libpkgstate.pc')],
  )
  test(
    'verify release metadata',
    sh,
    args: [join_paths(meson.current_source_dir(),
                      'release_metadata_test.sh'),
           meson.project_source_root()],
  )

  if get_option('tools').allowed()
    pkgstate_check_fixture = executable(
      'pkgstate-check-fixture',
      'pkgstate_check_fixture.cpp',
      dependencies: libpkgstate_dep,
    )
    test(
      'exercise read-only native state diagnostics',
      sh,
      args: [join_paths(meson.current_source_dir(),
                       'pkgstate_check_cli_test.sh'),
             pkgstate_check_tool, pkgstate_check_fixture],
    )
    test(
      'verify pkgstate-check manual contract',
      sh,
      args: [join_paths(meson.current_source_dir(),
                       'pkgstate_check_manual_test.sh'),
             meson.project_source_root(), pkgstate_check_tool],
    )
  endif
endif
