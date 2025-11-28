if cfitsio_dep.found()
  plugin_name = 'file-fits'

  plugin_sourcecode = [
    'fits.c',
  ]
  plugin_sources = plugin_sourcecode

  if platform_windows
    plugin_sources += windows.compile_resources(
      gimp_plugins_rc,
      args: [
        rc_define_flag, rc_define_fmt.format('ORIGINALFILENAME_STR', plugin_name+'.exe'),
        rc_define_flag, rc_define_fmt.format('INTERNALNAME_STR', plugin_name),
      ],
      include_directories: [
        rootInclude, appInclude,
      ],
    )
  endif

  plugin_exe = executable(plugin_name,
                          plugin_sources,
                          dependencies: [
                           libgimpui_dep,
                           cfitsio_dep,
                           math,
                          ],
                          win_subsystem: 'windows',
                          install: true,
                          install_dir: gimpplugindir / 'plug-ins' / plugin_name)
  plugin_executables += [plugin_exe.full_path()]
endif
