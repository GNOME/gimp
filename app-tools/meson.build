
gimp_debug_tool = executable('gimp-debug-tool',
  'gimp-debug-tool.c',
  include_directories: rootInclude,
  dependencies: [
    gio,
    gegl,
    gtk2,
  ],
  link_with: [
    libapp,
    libappwidgets,
  ],
)
