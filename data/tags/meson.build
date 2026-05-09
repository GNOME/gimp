gimp_tags_default = i18n.merge_file(
  input : 'gimp-tags-default.xml.in',
  output: '@BASENAME@',
  data_dirs: meson.current_source_dir(),
  po_dir: potags_dir,
  install: true,
  install_dir: gimpdatadir / 'tags',
)

if xmllint.found()
  #Copy the DTD to the build directory so xmllint can find it without --path
  #(--path is limited, does not work if the environment have paths with spaces)
  gimp_tags_dtd = configure_file(
    input: 'gimp-tags.dtd',
    output: 'gimp-tags.dtd',
    copy: true
  )

  custom_target('validate-tags',
    command: [
      xmllint,
      '--output', '@OUTPUT@',
      '--valid', gimp_tags_default,
    ],
    depend_files: gimp_tags_dtd,
    output: [ 'validate-tags-output.xml' ],
    build_by_default: true,
    install: false
  )
endif
