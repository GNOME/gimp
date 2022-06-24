gimp_tips = i18n.merge_file(
  input : 'gimp-tips.xml.in',
  output: '@BASENAME@',
  data_dirs: meson.current_source_dir(),
  po_dir: potips_dir,
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
  custom_target('validate-tips',
    command: [
      xmllint,
      '--output', '@OUTPUT@',
      '--path', meson.current_source_dir(),
      '--valid', gimp_tips,
  ],
    output: [ 'validate-tips-output.xml' ],
    build_by_default: true,
    install: false
  )
endif
