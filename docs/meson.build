
# Configure_file is just here to rename the files
manconf = configuration_data()
manconf.set('GIMP_VERSION',   gimp_version)
manconf.set('GIMP_APP_VERSION', gimp_app_version)
manconf.set('gimpdatadir',    prefix / gimpdatadir)
manconf.set('gimplocaledir',  prefix / localedir)
manconf.set('gimpplugindir',  prefix / gimpplugindir)
manconf.set('manpage_gimpdir', '$XDG_CONFIG_HOME/' + gimpdir / gimp_app_version)
manconf.set('gimpsysconfdir', prefix / gimpsysconfdir)
manconf.set('mypaint_brushes_dir', mypaint_brushes_dir)


man_files = [
  configure_file(
    input : 'gimptool.1.in',
    output: 'gimptool-' + gimp_app_version + '.1',
    configuration: manconf,
  ),
  configure_file(
    input : 'gimprc.5.in',
    output: 'gimprc-' + gimp_app_version + '.5',
    configuration: manconf,
  ),
]

if enable_console_bin
  man_files += configure_file(
    input : 'gimp.1.in',
    output: 'gimp-console-' + gimp_app_version + '.1',
    configuration: manconf,
  )
endif

man_files +=  configure_file(
  input : 'gimp.1.in',
  output: 'gimp-' + gimp_app_version + '.1',
  configuration: manconf,
)

install_man(man_files)

if enable_default_bin and not platform_windows
  install_symlink('gimptool.1',
    pointing_to: 'gimptool-' + gimp_app_version + '.1',
    install_dir: get_option('mandir') + '/man1'
  )
  install_symlink('gimprc.5',
    pointing_to: 'gimprc-' + gimp_app_version + '.5',
    install_dir: get_option('mandir') + '/man5'
  )
  if enable_console_bin
    install_symlink('gimp-console.1',
      pointing_to: 'gimp-console-' + gimp_app_version + '.1',
      install_dir: get_option('mandir') + '/man1'
    )
  endif
  install_symlink('gimp.1',
    pointing_to: 'gimp-' + gimp_app_version + '.1',
    install_dir: get_option('mandir') + '/man1'
  )
endif
