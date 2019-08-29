plugin_name = 'selection-to-path'

plugin_sources = [
  'curve.c',
  'edge.c',
  'fit.c',
  'math.c',
  'pxl-outline.c',
  'selection-to-path-dialog.c',
  'selection-to-path.c',
  'spline.c',
  'vector.c',
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

plugin_selection_to_path_link = [
  libgimp,
  libgimpbase,
  libgimpcolor,
  libgimpconfig,
  libgimpmath,
  libgimpui,
  libgimpwidgets,
]
plugin_selection_to_path_deps = [
  gegl, gtk3, math,
]

selection_to_path = executable(plugin_name,
  plugin_sources,
  include_directories: [ rootInclude, ],
  link_with:    plugin_selection_to_path_link,
  dependencies: plugin_selection_to_path_deps,
  install: true,
  install_dir: gimpplugindir / 'plug-ins' / plugin_name,
)
