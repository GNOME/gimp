plugin_name = 'file-fli'

plugin_sources = [
  'fli-gimp.c',
  'fli.c',
]

if platform_win32
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
  c_args: '-DGIMP_DEPRECATED_REPLACE_NEW_API',
  install: true,
  install_dir: gimpplugindir / 'plug-ins' / plugin_name,
)
