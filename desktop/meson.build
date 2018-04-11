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
  install_data(join_paths(sizedir, 'gimp.png'),
    install_dir: join_paths(
      get_option('datadir'), 'icons', 'hicolor', sizedir, 'apps'
    )
  )
endforeach

desktopfilename = 'gimp.desktop'

desktop_conf = configuration_data()
desktop_conf.set('GIMP_APP_VERSION', app_version)
desktop_conf.set('GIMP_COMMAND', gimp_command)
desktop_conf.set('GIMP_VERSION', gimp_version)
desktop_conf.set('MIME_TYPES', ';'.join(MIMEtypes))

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
    '--cache='+join_paths('@OUTDIR@', 'intltool-merge-cache'),
  ],
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications'),
)

appdatafilename = 'org.gimp.GIMP.appdata.xml'
appdatafile = custom_target(appdatafilename,
  input : [ appdatafilename+'.in.in', ],
  output: [ appdatafilename, ],
  command: [
    intltool_merge,
    po_dir,
    '@INPUT@',
    '@OUTPUT@',
    '--xml-style',
    '--utf8',
    '--cache='+join_paths('@OUTDIR@', 'intltool-merge-cache'),
  ],
  install: true,
  install_dir: join_paths(get_option('datadir'), 'metainfo'),
)

metainfofilename = 'gimp-data-extras.metainfo.xml'
metainfofile = custom_target(metainfofilename,
  input : [ metainfofilename+'.in', ],
  output: [ metainfofilename, ],
  command: [
    intltool_merge,
    po_dir,
    '@INPUT@',
    '@OUTPUT@',
    '--xml-style',
    '--utf8',
    '--cache='+join_paths('@OUTDIR@', 'intltool-merge-cache'),
  ],
  install: true,
  install_dir: join_paths(get_option('datadir'), 'appdata'),
)

custom_target('validate-desktop',
  input : [ desktopfile, ],
  output: [ 'validate-desktop' ],
  command: [
    desktop_validate, '@INPUT@',
  ],
  build_always: false,
  build_by_default: false,
)

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
endif
