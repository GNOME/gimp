
libgimpmath_sources = [
  'gimpmatrix.c',
  'gimpmd5.c',
  'gimpvector.c',
]

libgimpmath_headers = [
  'gimpmath.h',
  'gimpmathtypes.h',
  'gimpmatrix.h',
  'gimpmd5.h',
  'gimpvector.h',
]


libgimpmath = library('gimpmath-' + api_version,
  libgimpmath_sources,
  include_directories: rootInclude,
  dependencies: [
    glib, gobject, math,
  ],
  c_args:     [
    '-DG_LOG_DOMAIN="LibGimpMath"',
    '-DGIMP_MATH_COMPILATION',
  ],
  link_with:  [
    libgimpbase,
  ],
  vs_module_defs: 'gimpmath.def',
  install: true,
  version: so_version,
)

install_headers(
  libgimpmath_headers,
  subdir: join_paths(gimp_api_name, 'libgimpmath'),
)
