
if platform_win32
  gimp_debug_resume = executable('gimp-debug-resume',
    'gimp-debug-resume.c',
  )
endif

tools_cflags = [
  '-DGIMPDATADIR=' +'"'+ join_paths(prefix, gimpdatadir) +'"',
]


gimptool = executable('gimptool-' + api_version,
  'gimptool.c',
  include_directories: rootInclude,
  dependencies: [
    gtk2,
  ],
  link_with: [
    libgimpbase,
  ],
  c_args: tools_cflags,
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
    gtk2,
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
