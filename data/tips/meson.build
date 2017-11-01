
xmlfilename = 'gimp-tips.xml'

gimp_tips = custom_target(xmlfilename,
  input : xmlfilename+'.in',
  output: xmlfilename,

  command: [
    intltool_merge,
    potips_dir,
    '@INPUT@',
    '@OUTPUT@',
    '--xml-style',
    '--utf8',
    '--cache=' + '@OUTDIR@' / 'intltool-merge-cache',
  ],
  install: true,
  install_dir: gimpdatadir / 'tips',
)

fortunes_tips = custom_target('fortunes-tips',
  input : [ gimp_tips, 'fortunes.xsl', ],
  output: 'fortunes-tips',
  command: [
    xsltproc,
    '--stringparam', 'lang', 'en',
    '--path', meson.current_source_dir(),
    '--output', '@OUTPUT@',
    '@INPUT1@',
    '@INPUT0@',
  ],
)

if xmllint.found()
  run_target('validate-tips',
    command: [
      xmllint,
      '--noout',
      '--path', meson.current_source_dir(),
      '--valid', gimp_tips,
    ],
  )
endif
