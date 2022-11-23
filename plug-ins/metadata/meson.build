
plugin_name = 'metadata-editor'

plugin_sources = [
  'metadata-editor.c',
  'metadata-impexp.c',
  'metadata-tags.c',
  'metadata-xml.c',
]

if platform_windows
  plugin_rc = configure_file(
    input : ligma_plugins_rc.full_path(),
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

executable(plugin_name,
  plugin_sources,
  dependencies: [
    libligmaui_dep,
    gexiv2,
  ],
  install: true,
  install_dir: ligmaplugindir / 'plug-ins' / plugin_name,
)

###############################################################################

plugin_name = 'metadata-viewer'

plugin_sources = [
  'metadata-viewer.c',
  'metadata-tags.c',
]

if platform_windows
  plugin_rc = configure_file(
    input : ligma_plugins_rc.full_path(),
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

executable(plugin_name,
  plugin_sources,
  include_directories: [ rootInclude, ],
  dependencies: [
    gtk3, gegl, gexiv2,
  ],
  link_with: [
    libligma,
    libligmabase,
    libligmacolor,
    libligmaconfig,
    libligmamath,
    libligmaui,
    libligmawidgets,
  ],
  install: true,
  install_dir: ligmaplugindir / 'plug-ins' / plugin_name,
)
