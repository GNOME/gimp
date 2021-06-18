
xmlfilename = 'gimp-tags-default.xml'

gimp_tags_default = custom_target(xmlfilename,
  input : xmlfilename+'.in',
  output: xmlfilename,

  command: [
    intltool_merge,
    potags_dir,
    '@INPUT@',
    '@OUTPUT@',
    '--xml-style',
    '--utf8',
    '--cache=' + '@OUTDIR@' / 'intltool-merge-cache',
  ],
  install: true,
  install_dir: gimpdatadir / 'tags',
)

if xmllint.found()
  custom_target('validate-tags',
    command: [
      xmllint,
      '--output', '@OUTPUT@',
      '--path', meson.current_source_dir(),
      '--valid', gimp_tags_default,
    ],
    output: [ 'validate-tags-output.xml' ],
    build_by_default: true,
    install: false
  )
endif
