env=environment()

if meson.can_run_host_binaries()
  if enable_console_bin
    gimp_exe=gimpconsole_exe
  else
    gimp_exe=gimpmain_exe
  endif

  # make GIMP runnable without being installed.

  menu_paths=meson.global_build_root() / 'menus:' + meson.global_source_root() / 'menus'
  env.set('GIMP_TESTING_MENUS_PATH', menu_paths)

  env.set('GIMP_TESTING_PLUGINDIRS', meson.global_build_root() / 'plug-ins')
  env.append('GIMP_TESTING_PLUGINDIRS', meson.global_build_root() / 'plug-ins/python')
  env.append('GIMP_TESTING_PLUGINDIRS', meson.global_build_root() / 'plug-ins/common/test-plug-ins/')

  env.set('GIMP_TESTING_INTERPRETER_DIRS', meson.global_build_root() / 'plug-ins/python/')
  env.set('GIMP_TESTING_ENVIRON_DIRS', meson.global_source_root() / 'data/environ/')

  env.prepend('GI_TYPELIB_PATH', prefix / 'lib/girepository-1.0/')
  env.prepend('GI_TYPELIB_PATH', meson.global_build_root() / 'libgimp')

  env.prepend('LD_LIBRARY_PATH', meson.global_build_root() / 'libgimp')
  env.prepend('LD_LIBRARY_PATH', meson.global_build_root() / 'libgimpbase')
  env.prepend('LD_LIBRARY_PATH', meson.global_build_root() / 'libgimpcolor')
  env.prepend('LD_LIBRARY_PATH', meson.global_build_root() / 'libgimpconfig')
  env.prepend('LD_LIBRARY_PATH', meson.global_build_root() / 'libgimpmath')
  env.prepend('LD_LIBRARY_PATH', meson.global_build_root() / 'libgimpmodule')
  env.prepend('LD_LIBRARY_PATH', meson.global_build_root() / 'libgimpthumb')
  env.prepend('LD_LIBRARY_PATH', meson.global_build_root() / 'libgimpwidgets')

  if platform_windows
    # On Windows, DLL are found through PATH rather than LD_LIBRARY_PATH.
    env.prepend('PATH', meson.global_build_root() / 'libgimp')
    env.prepend('PATH', meson.global_build_root() / 'libgimpbase')
    env.prepend('PATH', meson.global_build_root() / 'libgimpcolor')
    env.prepend('PATH', meson.global_build_root() / 'libgimpconfig')
    env.prepend('PATH', meson.global_build_root() / 'libgimpmath')
    env.prepend('PATH', meson.global_build_root() / 'libgimpmodule')
    env.prepend('PATH', meson.global_build_root() / 'libgimpthumb')
    env.prepend('PATH', meson.global_build_root() / 'libgimpwidgets')
  endif
else
  # Cross-builds require an existing native GIMP installed, unless we
  # can run target binaries.
  gimp_exe = find_program([gimpconsole_exe_name, gimpmain_exe_name],
                          native: true, required: false)

  if not gimp_exe.found()
    error('When cross-compiling, the build requires either an exe_wrapper or a native GIMP installed (@0@ or @1@).'.format(gimpconsole_exe_name, gimpmain_exe_name))
  endif
endif

## Splash Image and Welcome Dialog ##

splash = custom_target('gimp-splash.png',
                       input : [ 'export-splash.py' ],
                       output: [ 'gimp-splash.png', ],
                       command: [ gimp_exe, '-nidfs',
                                  files('gimp-splash.xcf.xz'),
                                  '--batch-interpreter', 'python-fu-eval',
                                  '-b', '-', '--quit'],
                       feed: true,
                       build_by_default: true,
                       env: env,
                       install_dir: gimpdatadir / 'images',
                       install: true)

## Windows Installer's Splash Image ##

if get_option('windows-installer')
  installsplash = custom_target('installsplash.bmp',
                                input : [ 'splash2installer.py' ],
                                output: [ 'stamp-installsplash.bmp', ],
                                command: [ gimp_exe, '-nidfs',
                                           files('gimp-splash.xcf.xz'),
                                           '--batch-interpreter', 'python-fu-eval',
                                           '-b', '-', '--quit'],
                                feed: true,
                                build_by_default: true,
                                env: env)
endif

subdir('logo')
