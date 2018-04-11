
if platform_win32
  gimp_debug_resume = executable('gimp-debug-resume',
    'gimp-debug-resume.c',
  )
endif


gimptool = executable('gimptool-' + user_version,
  'gimptool.c',
  include_directories: rootInclude,
  dependencies: [
    gtk3,
  ],
  link_with: [
    libgimpbase,
  ],
  c_args: [
    '-DDATADIR="@0@"'.format(datadir),
  ],
  install: true,
)

executable('kernelgen',
  'kernelgen.c',
  include_directories: rootInclude,
  install: false,
)

executable('gimp-test-clipboard',
  'gimp-test-clipboard.c',
  include_directories: rootInclude,
  dependencies: [
    gtk3,
  ],
  install: true,
)
