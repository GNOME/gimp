
# Configure_file is just here to rename the files
manconf = configuration_data()
manconf.set('LIGMA_VERSION',   ligma_version)
manconf.set('ligmadatadir',    prefix / ligmadatadir)
manconf.set('ligmalocaledir',  prefix / localedir)
manconf.set('ligmaplugindir',  prefix / ligmaplugindir)
manconf.set('manpage_ligmadir', '$XDG_CONFIG_HOME/' + ligmadir / ligma_app_version)
manconf.set('ligmasysconfdir', prefix / ligmasysconfdir)
manconf.set('mypaint_brushes_dir', mypaint_brushes_dir)


man_files = [
  configure_file(
    input : 'ligmatool.1.in',
    output: 'ligmatool-' + ligma_app_version + '.1',
    configuration: manconf,
  ),
  configure_file(
    input : 'ligmarc.5.in',
    output: 'ligmarc-' + ligma_app_version + '.5',
    configuration: manconf,
  ),
]

if enable_console_bin
  man_files += configure_file(
    input : 'ligma.1.in',
    output: 'ligma-console-' + ligma_app_version + '.1',
    configuration: manconf,
  )
endif

man_files +=  configure_file(
  input : 'ligma.1.in',
  output: 'ligma-' + ligma_app_version + '.1',
  configuration: manconf,
)

install_man(man_files)

if enable_default_bin and meson.version().version_compare('>=0.61.0')
  install_symlink('ligmatool.1',
    pointing_to: 'ligmatool-' + ligma_app_version + '.1',
    install_dir: get_option('mandir') + '/man1'
  )
  install_symlink('ligmarc.5',
    pointing_to: 'ligmarc-' + ligma_app_version + '.5',
    install_dir: get_option('mandir') + '/man5'
  )
  if enable_console_bin
    install_symlink('ligma-console.1',
      pointing_to: 'ligma-console-' + ligma_app_version + '.1',
      install_dir: get_option('mandir') + '/man1'
    )
  endif
  install_symlink('ligma.1',
    pointing_to: 'ligma-' + ligma_app_version + '.1',
    install_dir: get_option('mandir') + '/man1'
  )
endif
