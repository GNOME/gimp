sizes = [
  '16',
  '22',
  '24',
  '32',
  '48',
  '64',
  '256',
]

foreach size : sizes
  sizedir = size+'x'+size
  install_data(sizedir / 'gimp.png',
    install_dir: get_option('datadir') / 'icons' / 'hicolor' / sizedir / 'apps'
  )
endforeach

desktop_conf = {
  'GIMP_COMMAND':     gimp_command,
  'GIMP_VERSION':     gimp_version,
  'GIMP_APP_VERSION': gimp_app_version,
  'MIME_TYPES':       ';'.join(MIMEtypes),
  'BUG_REPORT_URL':   bug_report_url,
}

desktopfile = i18n.merge_file(
  input: configure_file(
    input: 'org.gimp.GIMP.desktop.in.in',
    output: '@BASENAME@',
    configuration: desktop_conf,
  ),
  output: '@BASENAME@',
  type: 'desktop',
  po_dir: po_dir,
  install: true,
  install_dir: get_option('datadir') / 'applications',
)

appdatafile = i18n.merge_file(
  input: configure_file(
    input: 'org.gimp.GIMP.appdata.xml.in.in',
    output: '@BASENAME@',
    configuration: desktop_conf,
  ),
  output: '@BASENAME@',
  type: 'xml',
  po_dir: po_dir,
  install: true,
  install_dir: get_option('datadir') / 'metainfo',
)

metainfofile = i18n.merge_file(
  input: configure_file(
    input: 'gimp-data-extras.metainfo.xml.in.in',
    output: '@BASENAME@',
    configuration: desktop_conf,
  ),
  output: '@BASENAME@',
  po_dir: po_dir,
  install: true,
  install_dir: get_option('datadir') / 'metainfo',
)

if desktop_validate.found()
  run_target('validate-desktop',
    command: [ desktop_validate, desktopfile ],
  )
endif

if appstream_util.found()
  test('appdata_file',
    appstream_util,
    args: [ 'validate-relax', appdatafile ],
    env: [
      'GIMP_TESTING_ABS_TOP_BUILDDIR=' + meson.build_root(),
    ],
    suite: 'desktop',
  )

  test('metainfo_file',
    appstream_util,
    args: [ 'validate-relax', metainfofile ],
    env: [
      'GIMP_TESTING_ABS_TOP_BUILDDIR=' + meson.build_root(),
    ],
    suite: 'desktop',
  )

  test('desktop_file',
    find_program('test-desktop.sh'),
    env: [
      'GIMP_TESTING_ABS_TOP_BUILDDIR=' + meson.build_root(),
    ],
    suite: 'desktop',
  )
endif
