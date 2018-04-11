etcconf = configuration_data()
etcconf.set('mypaint_brushes_dir', libmypaint_brushes_dir)


install_data(
  [
    'controllerrc',
    configure_file(
      input : 'gimprc.in',
      output: 'gimprc',
      configuration: etcconf,
    ),
    'gimp.css',
    'menurc',
    'sessionrc',
    'templaterc',
    'unitrc',
  ],
  install_dir: gimpsysconfdir,
)
