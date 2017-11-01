
libgimpmath_sources_introspectable = files(
  'gimpmatrix.c',
  'gimpvector.c',
)

libgimpmath_sources = [
  libgimpmath_sources_introspectable,
]

libgimpmath_headers_introspectable = files(
  'gimpmathtypes.h',
  'gimpmatrix.h',
  'gimpvector.h',
)

libgimpmath_headers = [
  libgimpmath_headers_introspectable,
  'gimpmath.h',
]

libgimpmath_introspectable = [
  libgimpmath_sources_introspectable,
  libgimpmath_headers_introspectable,
]


libgimpmath = library('gimpmath-' + gimp_api_version,
  libgimpmath_sources,
  include_directories: rootInclude,
  dependencies: [
    glib, gobject, math,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibGimpMath"', '-DGIMP_MATH_COMPILATION', ],
  link_with: [
    libgimpbase,
  ],
  vs_module_defs: 'gimpmath.def',
  install: true,
  version: so_version,
)

install_headers(
  libgimpmath_headers,
  subdir: gimp_api_name / 'libgimpmath',
)
