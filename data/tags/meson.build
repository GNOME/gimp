ligma_tags_default = i18n.merge_file(
  input : 'ligma-tags-default.xml.in',
  output: '@BASENAME@',
  data_dirs: meson.current_source_dir(),
  po_dir: potags_dir,
  install: true,
  install_dir: ligmadatadir / 'tags',
)

if xmllint.found()
  custom_target('validate-tags',
    command: [
      xmllint,
      '--output', '@OUTPUT@',
      '--path', meson.current_source_dir(),
      '--valid', ligma_tags_default,
    ],
    output: [ 'validate-tags-output.xml' ],
    build_by_default: true,
    install: false
  )
endif
