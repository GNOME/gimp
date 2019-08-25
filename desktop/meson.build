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

desktopfilename = 'gimp.desktop'

desktop_conf = configuration_data()
desktop_conf.set('GIMP_COMMAND',      gimp_command)
desktop_conf.set('GIMP_VERSION',      gimp_version)
desktop_conf.set('GIMP_APP_VERSION',  gimp_app_version)
desktop_conf.set('MIME_TYPES',        ';'.join(MIMEtypes))
desktop_conf.set('BUG_REPORT_URL',    bug_report_url)

desktopfilein = configure_file(
  input : desktopfilename+'.in.in',
  output: desktopfilename+'.in',
  configuration: desktop_conf,
)

desktopfile = custom_target(desktopfilename,
  input : [ desktopfilein, ],
  output: [ desktopfilename, ],
  command: [
    intltool_merge,
    po_dir,
    '@INPUT@',
    '@OUTPUT@',
    '--desktop-style',
    '--utf8',
    '--cache=' + '@OUTDIR@' / 'intltool-merge-cache',
  ],
  install: true,
  install_dir: get_option('datadir') / 'applications',
)

appdatafilename = 'org.gimp.GIMP.appdata.xml'
appdatafilein = configure_file(
  input : appdatafilename+'.in.in',
  output: appdatafilename+'.in',
  configuration: desktop_conf,
)
appdatafile = custom_target(appdatafilename,
  input : [ appdatafilein, ],
  output: [ appdatafilename, ],
  command: [
    intltool_merge,
    po_dir,
    '@INPUT@',
    '@OUTPUT@',
    '--xml-style',
    '--utf8',
    '--cache=' + '@OUTDIR@' / 'intltool-merge-cache',
  ],
  install: true,
  install_dir: get_option('datadir') / 'metainfo',
)

metainfofilename = 'gimp-data-extras.metainfo.xml'
metainfofilein = configure_file(
  input : metainfofilename+'.in.in',
  output: metainfofilename+'.in',
  configuration: desktop_conf,
)
metainfofile = custom_target(metainfofilename,
  input : [ metainfofilein, ],
  output: [ metainfofilename, ],
  command: [
    intltool_merge,
    po_dir,
    '@INPUT@',
    '@OUTPUT@',
    '--xml-style',
    '--utf8',
    '--cache=' + '@OUTDIR@' / 'intltool-merge-cache',
  ],
  install: true,
  install_dir: get_option('datadir') / 'appdata',
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
