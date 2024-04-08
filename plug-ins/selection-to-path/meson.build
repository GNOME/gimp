plugin_name = 'selection-to-path'

plugin_sources = [
  'curve.c',
  'edge.c',
  'fit.c',
  'math.c',
  'pxl-outline.c',
  'selection-to-path.c',
  'spline.c',
  'vector.c',
]

if platform_windows
  plugin_sources += windows.compile_resources(
    gimp_plugins_rc,
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

plugin_selection_to_path_deps = [
  libgimpui_dep,
  math,
]

selection_to_path = executable(plugin_name,
                               plugin_sources,
                               dependencies: plugin_selection_to_path_deps,
                               install: true,
                               install_dir: gimpplugindir / 'plug-ins' / plugin_name)
plugin_executables += [selection_to_path.full_path()]
