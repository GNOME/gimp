desktopfilename = gimp_desktop_name + '.desktop'

desktop_conf = configuration_data()
desktop_conf.set('GIMP_COMMAND',      gimp_command)
desktop_conf.set('GIMP_VERSION',      gimp_version)
desktop_conf.set('GIMP_APP_VERSION',  gimp_app_version)
desktop_conf.set('GIMP_DESKTOP_NAME', gimp_desktop_name)
desktop_conf.set('MIME_TYPES',        'image/x-xcf;' + MIMEtypes_dep.get_variable('MIMEtypes'))
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

## The AppStream metadata goes through 3 transformations:
appdatafilename = 'org.gimp.GIMP.appdata.xml'
# 1. Replacing variables at configure time.
appdatafilein = configure_file(input : appdatafilename+'.in.in.in',
                               output: appdatafilename+'.in.in',
                               configuration: desktop_conf)
# 2. Removing our custom "demo" attributes (used in app/dialogs/meson.build).
cleaned_appdatafile = custom_target(appdatafilename+'.in',
                                    input : [ 'appstream.xsl', appdatafilein],
                                    output : [ appdatafilename+'.in' ],
                                    command: [ xsltproc, '-o', '@OUTPUT@', '@INPUT0@', '@INPUT1@' ])
# 3. Localization.
appdatafile = i18n.merge_file(input : cleaned_appdatafile,
                              output: appdatafilename,
                              po_dir: po_dir,
                              install: true,
                              install_dir: get_option('datadir') / 'metainfo')

if desktop_validate.found()
  test('validate-desktop',
    desktop_validate,
    args: [ desktopfile ],
    suite: 'desktop',
  )
endif

if appstreamcli.found()
  test('appdata_file',
    find_program('test-appdata.py', required: true, dirs: [meson.current_source_dir()]),
    env: [
      'GIMP_TESTING_BUILDDIR=' + meson.current_build_dir(),
      'GIMP_RELEASE=' + (release and not gimp_rc_git ? '1' : '0'),
    ],
    suite: 'desktop',
  )
endif
