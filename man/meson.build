# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

scdoc = find_program('scdoc', required: get_option('man_pages'))
if scdoc.found()
  manual_pages = [
    ['libpkgstate.3', '3'],
    ['pkgstate_model.3', '3'],
    ['pkgstate_store.3', '3'],
    ['pkgstate_canonical_generation_store.3', '3'],
    ['pkgstate_legacy_compatibility.3', '3'],
    ['pkgstate_legacy_import.3', '3'],
    ['pkgstate_legacy_text_store.3', '3'],
    ['pkgstate-db.5', '5'],
    ['pkgstate-generation.5', '5'],
  ]

  foreach page : manual_pages
    custom_target(
      page[0],
      input: page[0] + '.scdoc',
      output: page[0],
      command: [scdoc],
      feed: true,
      capture: true,
      install: true,
      install_dir: get_option('mandir') / ('man' + page[1]),
    )
  endforeach

  custom_target(
    'pkginfo.1',
    input: 'pkginfo.1.scdoc',
    output: 'pkginfo.1',
    command: [scdoc],
    feed: true,
    capture: true,
    install: get_option('install_tools'),
    install_dir: get_option('mandir') / 'man1',
  )
endif
