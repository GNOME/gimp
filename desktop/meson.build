sizes = [
  '16',
  '22',
  '24',
  '32',
  '48',
  '64',
  '256',
]


# XXX: the old autotools build used to `touch` the (datadir) /icons/hicolor
# folder at install time in order to comply with the "Icon Theme Specification"
# (see the "Implementation Notes" section about changing the mtime of the
# toplevel icon directory after installing/updating icon themes).
# This part was not reimplemented in our meson build. Unsure if this is still
# needed or even if this is the role of the application build to take care of
# this.

foreach size : sizes
  sizedir = size+'x'+size
  install_data(sizedir / 'gimp.png',
    install_dir: get_option('datadir') / 'icons' / 'hicolor' / sizedir / 'apps'
  )
endforeach
install_data('scalable/gimp.svg',
  install_dir: get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps'
)

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

desktopfile = i18n.merge_file(
  input : desktopfilein,
  output: desktopfilename,
  type: 'desktop',
  po_dir: po_dir,
  install: true,
  install_dir: get_option('datadir') / 'applications',
)

appdatafilename = 'org.gimp.GIMP.appdata.xml'
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
   # TODO: should we use validate-strict when the last errors for a strict
   # validation are fixed?
  test('appdata_file',
    appstreamcli,
    args: [ 'validate', appdatafile ],
    env: [
      'GIMP_TESTING_ABS_TOP_BUILDDIR=' + meson.project_build_root(),
    ],
    suite: 'desktop',
  )
endif
