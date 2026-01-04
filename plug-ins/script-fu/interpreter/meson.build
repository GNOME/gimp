
scriptfuInclude = include_directories('..')

executable_name = 'gimp-script-fu-interpreter-' + gimp_api_version

plugin_sourcecode = [
  'script-fu-interpreter.c',
  'script-fu-interpreter-plugin.c',
]
plugin_sources = plugin_sourcecode

if not meson.is_cross_build()
  if platform_windows
    plugin_sources += windows.compile_resources(
      plugin_rc,
      args: [
        rc_define_flag, rc_define_fmt.format('ORIGINALFILENAME_STR', executable_name+'.exe'),
        rc_define_flag, rc_define_fmt.format('INTERNALNAME_STR', executable_name),
      ],
      include_directories: [
        rootInclude, appInclude,
      ],
    )
  endif

  # !!! Installs as a usual binary say to /usr/bin, unlike extension-script-fu
  # GIMP queries scripts with shebangs, which invokes gimp-script-fu-interpreter-3.0.

  script_fu_interpreter = executable(executable_name,
                                     plugin_sources,
                                     dependencies: [
                                       libgimpui_dep,
                                       math,
                                     ],
                                     c_args: [
                                       '-DG_LOG_DOMAIN="scriptfu"',
                                     ],
                                     include_directories: [
                                       scriptfuInclude,
                                     ],
                                     link_with : libscriptfu,
                                     win_subsystem: 'windows',
                                     install: true)

  # Ugly trick to add executables to plugin_custom_targets list (see root meson.build)
  plugin_custom_targets += custom_target('dummy-' + plugin_name,
              input: plugin_exe,
              output: 'dummy-' + plugin_name + '.dummy',
              command: [python, '-c',
                        'open("@OUTPUT@", "w").write("Nice. I was built before calling in-build-gimp.py\\n")'],
              build_by_default: true)
  plugin_executables += [plugin_exe.full_path()]
endif
