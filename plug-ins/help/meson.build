plugin_name = 'help'

plugin_sources = [
  # 'ligma-help-lookup.c',
  'ligmahelp.c',
  'ligmahelpdomain.c',
  'ligmahelpitem.c',
  'ligmahelplocale.c',
  'ligmahelpprogress.c',
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

help_plugin_lib = static_library('help_plugin',
  plugin_sources,
  include_directories: [ rootInclude, ],
  dependencies: [
    gtk3, gegl, cairo, gio, gdk_pixbuf,
  ],
  install: false,
)

executable(plugin_name,
  'help.c',
  dependencies: [
    libligmaui_dep,
    gio,
    gdk_pixbuf,
  ],
  link_with: [
    help_plugin_lib,
  ],
  install: true,
  install_dir: ligmaplugindir / 'plug-ins' / plugin_name,
)
