etcconf = configuration_data()
etcconf.set('mypaint_brushes_dir', mypaint_brushes_dir)


install_data(
  [
    'controllerrc',
    configure_file(
      input : 'ligmarc.in',
      output: 'ligmarc',
      configuration: etcconf,
    ),
    'ligma.css',
    'menurc',
    'sessionrc',
    'templaterc',
    'toolrc',
    'unitrc',
  ],
  install_dir: ligmasysconfdir,
)
