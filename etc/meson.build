etcconf = configuration_data()


install_data(
  [
    'controllerrc',
    configure_file(
      input : 'gimprc.in',
      output: 'gimprc',
      configuration: etcconf,
    ),
    'gtkrc',
    'menurc',
    'sessionrc',
    'templaterc',
    'unitrc',
  ],
  install_dir: gimpsysconfdir,
)
