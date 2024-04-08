
file_raw_exes = [
  'file-darktable',
  'file-raw-placeholder',
  'file-rawtherapee',
]

foreach plugin_name : file_raw_exes

  plugin_sources = [
    plugin_name +'.c',
    'file-raw-utils.c',
  ]

  if platform_windows
    plugin_rc = configure_file(
      input : gimp_plugins_rc.full_path(),
      output: plugin_name + '.rc',
      copy: true,
    )
    plugin_sources += windows.compile_resources(
      plugin_rc,
      args: [
        '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plugin_name+'.exe'),
        '--define', 'INTERNALNAME_STR="@0@"'    .format(plugin_name),
        '--define', 'TOP_SRCDIR="@0@"'          .format(meson.project_source_root()),
      ],
      include_directories: [
        rootInclude, appInclude,
      ],
    )
  endif


  plugin_exe = executable(plugin_name,
                          plugin_sources,
                          dependencies: libgimpui_dep,
                          install: true,
                          install_dir: gimpplugindir / 'plug-ins' / plugin_name)
  plugin_executables += [plugin_exe.full_path()]
endforeach

install_data([
    'file-darktable-export-on-exit.lua',
    'file-darktable-get-size.lua',
  ],
  install_dir: prefix / gimpdatadir / 'file-raw',
)
