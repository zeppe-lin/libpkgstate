# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

scdoc = find_program('scdoc', required: get_option('man_pages'))
if scdoc.found()
  manual_pages = [
    ['libpkgstate.3', '3'],
    ['pkgstate_authority.7', '7'],
    ['pkgstate_model.3', '3'],
    ['pkgstate_installation_receipt.3', '3'],
    ['pkgstate_publication.3', '3'],
    ['pkgstate_store.3', '3'],
    ['pkgstate_canonical_generation_store.3', '3'],
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
    'pkgstate-check.1',
    input: 'pkgstate-check.1.scdoc',
    output: 'pkgstate-check.1',
    command: [scdoc],
    feed: true,
    capture: true,
    install: get_option('install_tools'),
    install_dir: get_option('mandir') / 'man1',
  )
endif
