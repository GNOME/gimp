# Extra markdown files
gimp_ui_doc_content_files = [
  'widget-gallery.md',
]

gimp_ui_doc_toml = configure_file(
  input: 'gimp-ui-3.0.toml.in',
  output: '@BASENAME@',
  configuration: {
    'GIMP_VERSION': gimp_version,
    'GIMP_LOGO': gimp_logo_dir / 'gimp-logo.png',
  },
)

gimp_ui_docs = custom_target('gimp-ui-docs',
  input: libgimpui_gir[0],
  output: 'GimpUi-@0@'.format(gimp_api_version),
  command: [
    gi_docgen,
    'generate',
    '--quiet',
    '--fatal-warnings',
    '--config', gimp_ui_doc_toml,
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '--add-include-path=@0@'.format(meson.build_root() / 'libgimp'),
    '@INPUT@',
  ],
  depends: libgimp_gir[0],
  depend_files: [
    gimp_ui_doc_toml,
    gimp_ui_doc_content_files,
  ],
  build_by_default: true,
  install: true,
  install_dir: get_option('datadir') / 'doc',
)
