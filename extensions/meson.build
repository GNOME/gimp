subdir('goat-exercises')

# Fallback fix to the problem of non-configured interpreters
if platform_windows and not meson.is_cross_build() and lua.found()
  lua_config = configuration_data()
  lua_config.set('LUA_PATH', '')

  configure_file(
    input : 'lua.interp.in',
    output: 'lua.interp',
    configuration: lua_config,
    install: true,
    install_dir: gimpplugindir / 'interpreters',
  )
endif