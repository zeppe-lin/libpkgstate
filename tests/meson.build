if get_option('tests').allowed()
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
endif
