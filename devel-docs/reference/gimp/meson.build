# Extra markdown files
ligma_doc_content_files = [
]

ligma_doc_toml = configure_file(
  input: 'ligma-3.0.toml.in',
  output: '@BASENAME@',
  configuration: {
    'LIGMA_VERSION': ligma_version,
    'LIGMA_LOGO_PATH': '../images/' + (stable ? 'ligma-logo.png' : 'ligma-devel-logo.png'),
  },
)

ligma_docs = custom_target('ligma-docs',
  input: libligma_gir[0],
  output: 'libligma-@0@'.format(ligma_api_version),
  command: [
    gi_docgen,
    'generate',
    '--quiet',
    '--fatal-warnings',
    '--config', ligma_doc_toml,
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '@INPUT@',
  ],
  depend_files: [
    ligma_doc_toml,
    ligma_doc_content_files,
  ],
  build_by_default: true,
  install: true,
  install_dir: get_option('datadir') / 'doc' / 'ligma-@0@'.format(ligma_app_version),
)
