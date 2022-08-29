
if platform_windows
  gimp_debug_resume = executable('gimp-debug-resume',
    'gimp-debug-resume.c',
  )
endif


gimptool = executable('gimptool-' + gimp_app_version,
  'gimptool.c',
  include_directories: rootInclude,
  dependencies: [
    gtk3,
  ],
  link_with: [
    libgimpbase,
  ],
  c_args: [
    '-DDATADIR="@0@"'.format(get_option('datadir')),
  ],
  install: true,
)

executable('gimp-test-clipboard-' + gimp_app_version,
  'gimp-test-clipboard.c',
  include_directories: rootInclude,
  dependencies: [
    gtk3,
  ],
  install: true,
)

if enable_default_bin and meson.version().version_compare('>=0.61.0')
  install_symlink('gimptool' + exec_ext,
    pointing_to: 'gimptool-' + gimp_app_version + exec_ext,
    install_dir: exec_dir
  )
  install_symlink('gimp-test-clipboard' + exec_ext,
    pointing_to: 'gimp-test-clipboard-' + gimp_app_version + exec_ext,
    install_dir: exec_dir
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
