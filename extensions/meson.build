subdir('goat-exercises')

#.interp file read by gimp_interpreter_db_load_interp_file in gimpinterpreterdb.c
if have_lua and not meson.is_cross_build() and is_variable('lua') and lua.found() and (platform_windows or not relocatable_bundle)
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

if have_javascript and (platform_windows or platform_osx) and not meson.is_cross_build()
  gjs_config = configuration_data()

  if platform_windows
    # gjs-console.exe is needed for plug-ins error output if `gimp*.exe` is run from console
    gjs_config.set('GJS_EXE', 'gjs-console.exe')

    # gjs.exe is needed to run plug-ins silently if `gimp*.exe` is run from shortcut
    install_data('gjs_win.interp',
      install_dir: gimpplugindir / 'interpreters',
    )
  elif platform_osx and relocatable_bundle
    # gjs is needed to run plug-ins with right bundled interpreter when relocatable
    gjs_config.set('GJS_EXE', 'gjs')
  endif

  configure_file(input : 'gjs.interp.in',
                 output: 'gjs.interp',
                 configuration: gjs_config,
                 install: true,
                 install_dir: gimpplugindir / 'interpreters')
endif
