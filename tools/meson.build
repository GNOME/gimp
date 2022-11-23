
if platform_windows
  ligma_debug_resume = executable('ligma-debug-resume',
    'ligma-debug-resume.c',
  )
endif


ligmatool = executable('ligmatool' + exec_ver,
  'ligmatool.c',
  include_directories: rootInclude,
  dependencies: [
    gtk3,
  ],
  link_with: [
    libligmabase,
  ],
  c_args: [
    '-DDATADIR="@0@"'.format(get_option('datadir')),
  ],
  install: true,
)

ligma_test_clipboard = executable('ligma-test-clipboard' + exec_ver,
  'ligma-test-clipboard.c',
  include_directories: rootInclude,
  dependencies: [
    gtk3,
  ],
  install: true,
)

if enable_default_bin and meson.version().version_compare('>=0.61.0')
  install_symlink(fs.name(ligmatool.full_path()).replace(exec_ver, ''),
    pointing_to: fs.name(ligmatool.full_path()),
    install_dir: get_option('bindir')
  )
  install_symlink(fs.name(ligma_test_clipboard.full_path()).replace(exec_ver, ''),
    pointing_to: fs.name(ligma_test_clipboard.full_path()),
    install_dir: get_option('bindir')
  )
endif

executable('kernelgen',
  'kernelgen.c',
  include_directories: rootInclude,
  install: false,
)

colorsvg2png = executable('colorsvg2png',
  'colorsvg2png.c',
  native: true,
  dependencies: [
    native_glib,
    native_rsvg
  ],
  install: false,
)
