subdir('goat-exercises')

if not meson.is_cross_build() and is_variable('lua') and lua.found() and (platform_windows or not relocatable_bundle)
  lua_config = configuration_data()
  # For Windows, we set the binary name only.
  # For other OSes, we use the full path (when not in a relocatable
  # bundle).
  if platform_windows
    lua_config.set('LUA_PATH', lua_bin + '.exe')
  else
    lua_config.set('LUA_PATH', lua.full_path())
  endif

  configure_file(input : 'lua.interp.in',
                 output: 'lua.interp',
                 configuration: lua_config,
                 install: true,
                 install_dir: gimpplugindir / 'interpreters')
endif
