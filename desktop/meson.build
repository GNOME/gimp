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
  install_data(sizedir / 'ligma.png',
    install_dir: get_option('datadir') / 'icons' / 'hicolor' / sizedir / 'apps'
  )
endforeach
install_data('scalable/ligma.svg',
  install_dir: get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps'
)

desktopfilename = 'ligma.desktop'

desktop_conf = configuration_data()
desktop_conf.set('LIGMA_COMMAND',      ligma_command)
desktop_conf.set('LIGMA_VERSION',      ligma_version)
desktop_conf.set('LIGMA_APP_VERSION',  ligma_app_version)
desktop_conf.set('MIME_TYPES',        ';'.join(MIMEtypes))
desktop_conf.set('BUG_REPORT_URL',    bug_report_url)

desktopfilein = configure_file(
  input : desktopfilename+'.in.in',
  output: desktopfilename+'.in',
  configuration: desktop_conf,
)

desktopfile = i18n.merge_file(
  input : desktopfilein,
  output: desktopfilename,
  type: 'desktop',
  po_dir: po_dir,
  install: true,
  install_dir: get_option('datadir') / 'applications',
)

appdatafilename = 'org.ligma.LIGMA.appdata.xml'
appdatafilein = configure_file(
  input : appdatafilename+'.in.in',
  output: appdatafilename+'.in',
  configuration: desktop_conf,
)
appdatafile = i18n.merge_file(
  input : appdatafilein,
  output: appdatafilename,
  po_dir: po_dir,
  install: true,
  install_dir: get_option('datadir') / 'metainfo',
)

if desktop_validate.found()
  test('validate-desktop',
    desktop_validate,
    args: [ desktopfile ],
    suite: 'desktop',
  )
endif

if appstreamcli.found()
  test('appdata_file',
    appstreamcli,
    args: [ 'validate', appdatafile ],
    env: [
      'LIGMA_TESTING_ABS_TOP_BUILDDIR=' + meson.project_build_root(),
    ],
    suite: 'desktop',
  )
endif
