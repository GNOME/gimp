
# Configure_file is just here to rename the files
manconf = configuration_data()
manconf.set('GIMP_VERSION',   gimp_version)
manconf.set('gimpdatadir',    join_paths(prefix, gimpdatadir))
manconf.set('gimplocaledir',  join_paths(prefix, localedir))
manconf.set('gimpplugindir',  join_paths(prefix, gimpplugindir))
manconf.set('manpage_gimpdir', '\fB$XDG_CONFIG_HOME\fP/' + join_paths(gimpdir, user_version))
manconf.set('gimpsysconfdir', gimpsysconfdir)


man_files = [
  configure_file(
    input : 'gimptool.1.in',
    output: 'gimptool-' + api_version + '.1',
    configuration: manconf,
  ),
  configure_file(
    input : 'gimptool.1.in',
    output: 'gimptool.1',
    configuration: manconf,
  ),

  configure_file(
    input : 'gimprc.5.in',
    output: 'gimprc-' + app_version + '.5',
    configuration: manconf,
  ),
  configure_file(
    input : 'gimprc.5.in',
    output: 'gimprc.5',
    configuration: manconf,
  ),
]

if enable_console_bin
  man_files += configure_file(
    input : 'gimp.1.in',
    output: 'gimp-console-' + app_version + '.1',
    configuration: manconf,
  )
  man_files += configure_file(
    input : 'gimp.1.in',
    output: 'gimp-console.1',
    configuration: manconf,
  )
endif

if enable_default_bin
  man_files +=  configure_file(
    input : 'gimp.1.in',
    output: 'gimp-' + app_version + '.1',
    configuration: manconf,
  )
  man_files +=  configure_file(
    input : 'gimp.1.in',
    output: 'gimp.1',
    configuration: manconf,
  )
endif

install_man(man_files)
