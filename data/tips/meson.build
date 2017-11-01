
xmlfilename = 'gimp-tips.xml'

gimp_tips = custom_target(xmlfilename,
  input : [ xmlfilename+'.in', ],
  output: [ xmlfilename, ],

  command: [
    intltool_merge,
    potips_dir,
    '@INPUT@',
    '@OUTPUT@',
    '--xml-style',
    '--utf8',
    '--cache='+join_paths('@OUTDIR@', 'intltool-merge-cache'),
  ],
  install: true,
  install_dir: join_paths(gimpdatadir, 'tips'),
)


custom_target('validate-tips',
  input : [ gimp_tips, ],
  output: [ 'validate-tips' ],
  command: [
    xmllint,
    '--noout',
    '--path', meson.current_source_dir(),
    '--valid', '@INPUT@',
  ],
  build_always: false,
  build_by_default: false,
)

custom_target('fortunes-tips',
  input : [ gimp_tips, 'fortunes.xsl', ],
  output: [ 'fortunes-tips' ],
  command: [
    xsltproc,
    '--stringparam', 'lang', 'en',
    '--path', meson.current_source_dir(),
    '--output', '@OUTPUT@',
    '@INPUT1@',
    '@INPUT0@',
  ],
  build_always: false,
  build_by_default: false,
)
