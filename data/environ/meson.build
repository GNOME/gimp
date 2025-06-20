#.env files read by gimp_environ_table_load_env_file in gimpenvirontable.c


install_data(
  'default.env',
  install_dir: gimpplugindir / 'environ',
)


if relocatable_bundle or platform_windows and not meson.is_cross_build()
  pygimp_config = configuration_data()
  if relocatable_bundle
    pygimp_config.set('DONTWRITEBYTECODE_SETTING', 'PYTHONDONTWRITEBYTECODE=1')
  else
    pygimp_config.set('DONTWRITEBYTECODE_SETTING', '')
  endif
  if platform_windows 
    pygimp_config.set('HIGHDPI_SETTING', '__COMPAT_LAYER=HIGHDPIAWARE')
  else
    pygimp_config.set('HIGHDPI_SETTING', '')
  endif

  configure_file(
    input : 'pygimp.env.in',
    output: 'pygimp.env',
    configuration: pygimp_config,
    install: true,
    install_dir: gimpplugindir / 'environ',
  )
endif
