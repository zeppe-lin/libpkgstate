if get_option('tests').allowed()
  model_test = executable(
    'model-test',
    'model_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('validate installed package state', model_test)

  umbrella_header_test = executable(
    'umbrella-header-test',
    'umbrella_header_test.cpp',
    dependencies: libpkgstate_dep,
  )
  test('compile umbrella header', umbrella_header_test)
endif
