plugin_name = 'file-icns'

plugin_sources = [
  'file-icns-data.c',
  'file-icns-load.c',
  'file-icns-save.c',
  'file-icns.c',
]

if platform_windows
  plugin_sources += windows.compile_resources(
    ligma_plugins_rc,
    args: [
      '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plugin_name+'.exe'),
      '--define', 'INTERNALNAME_STR="@0@"'    .format(plugin_name),
      '--define', 'TOP_SRCDIR="@0@"'          .format(meson.source_root()),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
endif

executable(plugin_name,
  plugin_sources,
  dependencies: [
    libligmaui_dep,
    libpng,
  ],
  install: true,
  install_dir: ligmaplugindir / 'plug-ins' / plugin_name,
)
