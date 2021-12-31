# Extra markdown files
gimp_doc_content_files = [
  'porting_guide.md',
  'porting_scriptfu_scripts.md',
]

gimp_doc_toml = configure_file(
  input: 'gimp-3.0.toml.in',
  output: '@BASENAME@',
  configuration: {
    'GIMP_VERSION': gimp_version,
    'GIMP_LOGO_PATH': gimp_logo_dir / 'gimp-logo.png',
  },
)

gimp_docs = custom_target('gimp-docs',
  input: libgimp_gir[0],
  output: 'Gimp-@0@'.format(gimp_api_version),
  command: [
    gi_docgen,
    'generate',
    '--quiet',
    '--fatal-warnings',
    '--config', gimp_doc_toml,
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '@INPUT@',
  ],
  depend_files: [
    gimp_doc_toml,
    gimp_doc_content_files,
  ],
  build_by_default: true,
  install: true,
  install_dir: get_option('datadir') / 'doc',
)
