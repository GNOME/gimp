if platform_windows or platform_osx
  ligma_debug_tool_dir = get_option('bindir')
else
  ligma_debug_tool_dir = get_option('libexecdir')
endif


ligma_debug_tool = executable('ligma-debug-tool' + exec_ver,
  'ligma-debug-tool.c',
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
    libligmabase,
  ],
  install: true,
  install_dir: ligma_debug_tool_dir
)

if enable_default_bin and meson.version().version_compare('>=0.61.0')
  install_symlink(fs.name(ligma_debug_tool.full_path()).replace(exec_ver, ''),
    pointing_to: fs.name(ligma_debug_tool.full_path()),
    install_dir: ligma_debug_tool_dir
  )
endif
