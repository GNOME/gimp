
file_raw_exes = [
  'file-another-rawtherapee',
  'file-darktable',
  'file-raw-placeholder',
  'file-rawtherapee',
]

foreach plugin_name : file_raw_exes

  plugin_sourcecode = [
    plugin_name +'.c',
    'file-raw-utils.c',
  ]
  plugin_sources = plugin_sourcecode

  if platform_windows
    plugin_rc = custom_target(
      plugin_name + '-rc',
      input: gimp_plugins_rc,
      output: plugin_name + '.rc',
      command: [python, '-c', 'import shutil, sys; shutil.copy(sys.argv[1], sys.argv[2])', '@INPUT@', '@OUTPUT@']
    )
    plugin_sources += windows.compile_resources(
      plugin_rc,
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
                          dependencies: libgimpui_dep,
                          win_subsystem: 'windows',
                          install: true,
                          install_dir: gimpplugindir / 'plug-ins' / plugin_name)

  # Ugly trick to add executables to plugin_custom_targets list (see root meson.build)
  plugin_custom_targets += custom_target('dummy-' + plugin_name,
              input: plugin_exe,
              output: 'dummy-' + plugin_name + '.dummy',
              command: [python, '-c',
                        'open("@OUTPUT@", "w").write("Nice. I was built before calling in-build-gimp.py\\n")'],
              build_by_default: true)
endforeach

install_data([
    'file-darktable-export-on-exit.lua',
    'file-darktable-get-size.lua',
  ],
  install_dir: prefix / gimpdatadir / 'file-raw',
)
