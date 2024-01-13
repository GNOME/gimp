etcconf = configuration_data()
etcconf.set('GIMP_APP_VERSION', gimp_app_version)
etcconf.set('mypaint_brushes_dir', mypaint_brushes_dir)


install_data(
  [
    'controllerrc',
    configure_file(
      input : 'gimprc.in',
      output: 'gimprc',
      configuration: etcconf,
    ),
    'gimp.css',
    'sessionrc',
    'templaterc',
    'toolrc',
    'unitrc',
  ],
  install_dir: gimpsysconfdir,
)
