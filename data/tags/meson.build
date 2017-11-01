
xmlfilename = 'gimp-tags-default.xml'

gimp_tags_default = custom_target(xmlfilename,
  input : [ xmlfilename+'.in', ],
  output: [ xmlfilename, ],

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


custom_target('validate-tags',
  input : [ gimp_tags_default, ],
  output: [ 'validate-tags' ],
  command: [
    xmllint,
    '--noout',
    '--path', meson.current_source_dir(),
    '--valid', '@INPUT@',
  ],
  build_always_stale: false,
  build_by_default: false,
)
