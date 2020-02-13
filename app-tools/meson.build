
gimp_debug_tool = executable('gimp-debug-tool-' + gimp_app_version,
  'gimp-debug-tool.c',
  include_directories: rootInclude,
  dependencies: [
    fontconfig,
    gio,
    gegl,
    gtk3,
  ],
  link_with: [
    libapp,
    libappwidgets,
    libgimpbase,
  ],
  install: true,
  install_dir: get_option('libexecdir')
)
