if platform_windows or platform_osx
  gimp_debug_tool_dir = get_option('bindir')
else
  gimp_debug_tool_dir = get_option('libexecdir')
endif


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
  install_dir: gimp_debug_tool_dir
)

if enable_default_bin and meson.version().version_compare('>=0.61.0')
  install_symlink('gimp-debug-tool' + exec_ext,
    pointing_to: 'gimp-debug-tool-' + gimp_app_version + exec_ext,
    install_dir: gimp_debug_tool_dir
  )
endif
