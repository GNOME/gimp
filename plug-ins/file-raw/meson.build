
file_raw_exes = [
  'file-darktable',
  'file-raw-placeholder',
  'file-rawtherapee',
]

foreach plugin_name : file_raw_exes

  rc_file = platform_win32 ? windows.compile_resources(
    gimp_plugins_rc,
    args: [
      '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plugin_name+'.exe'),
      '--define', 'INTERNALNAME_STR="@0@"'    .format(plugin_name),
      '--define', 'TOP_SRCDIR="@0@"'          .format(meson.source_root()),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  ) : []

  executable(plugin_name,
    [
      plugin_name +'.c',
      'file-raw-utils.c',
      rc_file,
    ],
    include_directories: [ rootInclude, ],
    dependencies: [
      gtk3, gegl,
    ],
    link_with: [
      libgimp,
      libgimpbase,
      libgimpcolor,
      libgimpconfig,
      libgimpmath,
      libgimpui,
      libgimpwidgets,
    ],
    install: true,
    install_dir: join_paths(gimpplugindir, 'plug-ins'),
  )
endforeach

install_data([
    'file-darktable-export-on-exit.lua',
    'file-darktable-get-size.lua',
  ],
  install_dir: join_paths(prefix, gimpdatadir, 'file-raw'),
)
