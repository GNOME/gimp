# Extra markdown files
gimp_doc_content_files = [
]

gimp_doc_toml = configure_file(
  input: 'gimp-3.0.toml.in',
  output: '@BASENAME@',
  configuration: {
    'GIMP_VERSION': gimp_version,
    'GIMP_LOGO_PATH': '../images/gimp-devel-logo.png',
  },
)

gimp_docs = custom_target('gimp-docs',
  input: libgimp_gir[0],
  output: 'libgimp-@0@'.format(gimp_api_version),
  command: [
    gi_docgen,
    'generate',
    '--quiet',
    '--fatal-warnings',
    '--config', gimp_doc_toml,
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '--add-include-path=@0@'.format(get_option('prefix') / 'share' / 'gir-1.0'),
    '@INPUT@',
  ],
  depend_files: [
    gimp_doc_toml,
    gimp_doc_content_files,
  ],
  build_by_default: true,
  install: true,
  install_dir: get_option('datadir') / 'doc' / 'gimp-@0@'.format(gimp_app_version),
)
