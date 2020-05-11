if webp_found

plugin_name = 'file-webp'

plugin_sources = [
  'file-webp-dialog.c',
  'file-webp-load.c',
  'file-webp-save.c',
  'file-webp.c',
]

if platform_windows
  plugin_sources += windows.compile_resources(
    gimp_plugins_rc,
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
    libgimpui_dep,
    gexiv2,
    webp_libs,
  ],
  install: true,
  install_dir: gimpplugindir / 'plug-ins' / plugin_name,
)

endif
