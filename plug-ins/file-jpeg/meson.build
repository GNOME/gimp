plugin_name = 'file-jpeg'

plugin_sources = [
  'jpeg-icc.c',
  'jpeg-load.c',
  'jpeg-quality.c',
  'jpeg-save.c',
  'jpeg-settings.c',
  'jpeg.c',
]

if platform_windows
  plugin_sources += windows.compile_resources(
    ligma_plugins_rc,
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

executable(plugin_name,
  plugin_sources,
  dependencies: [
    libligmaui_dep,
    libjpeg,
    lcms,
    gexiv2,
  ],
  install: true,
  install_dir: ligmaplugindir / 'plug-ins' / plugin_name,
)
