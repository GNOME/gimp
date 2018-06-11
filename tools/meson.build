
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

if have_vector_icons
  invert_svg = generator(
    executable('invert-svg',
      'invert-svg.c',
      dependencies: gio,
    ),
    output: '@BASENAME@',
    arguments: [ '@INPUT@', '@OUTPUT@' ],
  )
endif
