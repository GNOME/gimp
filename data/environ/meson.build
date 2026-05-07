#.env files read by gimp_environ_table_load_env_file in gimpenvirontable.c


#Minimum runtime paths for non relocatable (aka Unix-style) builds
#This is needed when custom -Dprefix is set; and no-op if not set
#See: https://developer.gimp.org/core/setup/build/#preparing-for-building
if not relocatable_bundle and (platform_windows or platform_osx) and not meson.is_cross_build()
  default_config = configuration_data()
  if platform_windows or platform_osx
    gimp_debug_tool_dir = prefix / get_option('bindir')
  else
    gimp_debug_tool_dir = prefix / get_option('libexecdir')
  endif
  default_config.set('SCRIPTFU_DEBUGTOOL_PATH', ': PATH=' + prefix / get_option('bindir') + ':' + gimp_debug_tool_dir)
  default_config.set('INTROSPECT_PATH', ': GI_TYPELIB_PATH=' + prefix / get_option('libdir') / 'girepository-1.0')

  configure_file(
    input : 'default.env.in',
    output: 'default.env',
    configuration: default_config,
    install: true,
    install_dir: gimpplugindir / 'environ',
  )
else
  install_data(
    'default.env',
    install_dir: gimpplugindir / 'environ',
  )
endif

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
