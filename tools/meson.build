
if platform_win32
  gimp_debug_resume = executable('gimp-debug-resume',
    'gimp-debug-resume.c',
  )
endif


gimptool = executable('gimptool-' + api_version,
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

executable('test-clipboard',
  'test-clipboard.c',
  include_directories: rootInclude,
  dependencies: [
    gtk3,
  ],
  install: false,
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
