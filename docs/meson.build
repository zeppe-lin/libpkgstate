# SPDX-FileCopyrightText: 2026 Alexandr Savca
# SPDX-License-Identifier: GPL-3.0-or-later

documentation_dir = get_option('datadir') / 'doc' / meson.project_name()

install_data(
  '../CONTRIBUTING.md',
  '../DESIGN.md',
  '../HISTORY.md',
  '../MAINTAINING.md',
  '../MIGRATION.md',
  '../README.md',
  '../TESTING.md',
  '../COPYING',
  '../COPYRIGHT',
  install_dir: documentation_dir,
  install_tag: 'doc',
)

install_data(
  'abi.md',
  'architecture.md',
  'code-style.md',
  'html.md',
  'integration.md',
  'manpage-markdown.md',
  'testing.md',
  install_dir: documentation_dir,
  install_tag: 'doc',
)

install_data(
  'history/3.0-adapter-extraction.md',
  install_dir: documentation_dir / 'history',
  install_tag: 'doc',
)

install_data(
  'man/libpkgstate.3.md',
  'man/pkgstate_authority.7.md',
  'man/pkgstate_generation_codec.3.md',
  'man/pkgstate_installation_receipt.3.md',
  'man/pkgstate_model.3.md',
  'man/pkgstate_publication.3.md',
  'man/pkgstate_store.3.md',
  install_dir: documentation_dir / 'man',
  install_tag: 'doc',
)

install_data(
  'assets/house.css',
  'assets/doxygen-extra.css',
  install_dir: documentation_dir / 'assets',
  install_tag: 'doc',
)

html_docs_enabled = get_option('html_docs').enabled()
html_docs_target = []
html_docs_output_dir = ''

if html_docs_enabled
  pandoc = find_program('pandoc', required: true)
  doxygen = find_program('doxygen', required: true)
  html_docs_builder = find_program(meson.project_source_root() / 'tools/build-html-docs.py')
  html_docs_checker = find_program(meson.project_source_root() / 'tools/check-html-docs.py')
  html_docs_installer = find_program(meson.project_source_root() / 'tools/install-html-docs.py')

  html_docs_output_dir = (
    meson.current_build_dir()
    / 'html'
    / meson.project_name()
    / meson.project_version()
  )
  html_docs_sources = files(
    '../CONTRIBUTING.md',
    '../DESIGN.md',
    '../HISTORY.md',
    '../MAINTAINING.md',
    '../MIGRATION.md',
    '../README.md',
    '../TESTING.md',
    'abi.md',
    'architecture.md',
    'code-style.md',
    'html.md',
    'integration.md',
    'manpage-markdown.md',
    'testing.md',
    'history/3.0-adapter-extraction.md',
    'man/libpkgstate.3.md',
    'man/pkgstate_authority.7.md',
    'man/pkgstate_generation_codec.3.md',
    'man/pkgstate_installation_receipt.3.md',
    'man/pkgstate_model.3.md',
    'man/pkgstate_publication.3.md',
    'man/pkgstate_store.3.md',
    'assets/house.css',
    'assets/doxygen-extra.css',
    'html-manifest.json',
    '../COPYING',
    '../COPYRIGHT',
    '../Doxyfile',
    '../tools/build-html-docs.py',
    '../tools/check-html-docs.py',
    '../tools/install-html-docs.py',
    '../tools/render-man-markdown.py',
    '../tools/check-man-markdown.py',
    '../include/libpkgstate/canonical_store.h',
    '../include/libpkgstate/digest.h',
    '../include/libpkgstate/error.h',
    '../include/libpkgstate/export.h',
    '../include/libpkgstate/generation_codec.h',
    '../include/libpkgstate/installation_receipt.h',
    '../include/libpkgstate/installed_control.h',
    '../include/libpkgstate/installed_package.h',
    '../include/libpkgstate/libpkgstate.h',
    '../include/libpkgstate/model.h',
    '../include/libpkgstate/owned_entry.h',
    '../include/libpkgstate/package_path.h',
    '../include/libpkgstate/package_release.h',
    '../include/libpkgstate/package_source_record.h',
    '../include/libpkgstate/publication_codec.h',
    '../include/libpkgstate/publication_receipt.h',
    '../include/libpkgstate/publication_request.h',
    '../include/libpkgstate/snapshot.h',
    '../include/libpkgstate/state_target_binding.h',
  )

  html_docs_target = custom_target(
    'html-docs',
    input: html_docs_sources,
    output: 'html-docs.stamp',
    command: [
      html_docs_builder,
      '--source-root', meson.project_source_root(),
      '--manifest', meson.current_source_dir() / 'html-manifest.json',
      '--output-dir', html_docs_output_dir,
      '--project-version', meson.project_version(),
      '--pandoc', pandoc.full_path(),
      '--doxygen', doxygen.full_path(),
      '--checker', html_docs_checker.full_path(),
      '--stamp', '@OUTPUT@',
    ],
    build_by_default: true,
  )

  meson.add_install_script(
    html_docs_installer,
    html_docs_target,
    html_docs_output_dir,
    (
      get_option('datadir')
      / 'htmldocs'
      / meson.project_name()
      / meson.project_version()
    ),
    install_tag: 'html-docs',
  )
endif
