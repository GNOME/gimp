
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
  run_target('validate-tags',
    command: [
      xmllint,
      '--noout',
      '--path', meson.current_source_dir(),
      '--valid', gimp_tags_default,
    ],
  )
endif
