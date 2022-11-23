# Extra markdown files
ligma_ui_doc_content_files = [
  'widget-gallery.md',
]

ligma_ui_doc_toml = configure_file(
  input: 'ligma-ui-3.0.toml.in',
  output: '@BASENAME@',
  configuration: {
    'LIGMA_VERSION': ligma_version,
    'LIGMA_LOGO': '../images/' + (stable ? 'ligma-logo.png' : 'ligma-devel-logo.png'),
  },
)

ligma_ui_docs = custom_target('ligma-ui-docs',
  input: libligmaui_gir[0],
  output: 'libligmaui-@0@'.format(ligma_api_version),
  command: [
    gi_docgen,
    'generate',
    '--quiet',
    '--fatal-warnings',
    '--config', ligma_ui_doc_toml,
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '--add-include-path=@0@'.format(meson.project_build_root() / 'libligma'),
    '@INPUT@',
  ],
  depends: libligma_gir[0],
  depend_files: [
    ligma_ui_doc_toml,
    ligma_ui_doc_content_files,
  ],
  build_by_default: true,
  install: true,
  install_dir: get_option('datadir') / 'doc' / 'ligma-@0@'.format(ligma_app_version),
)
