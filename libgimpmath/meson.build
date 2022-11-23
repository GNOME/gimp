
libligmamath_sources_introspectable = files(
  'ligmamatrix.c',
  'ligmavector.c',
)

libligmamath_sources = [
  libligmamath_sources_introspectable,
]

libligmamath_headers_introspectable = files(
  'ligmamathtypes.h',
  'ligmamatrix.h',
  'ligmavector.h',
)

libligmamath_headers = [
  libligmamath_headers_introspectable,
  'ligmamath.h',
]

libligmamath_introspectable = [
  libligmamath_sources_introspectable,
  libligmamath_headers_introspectable,
]


libligmamath = library('ligmamath-' + ligma_api_version,
  libligmamath_sources,
  include_directories: rootInclude,
  dependencies: [
    glib, gobject, math,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibLigmaMath"', '-DLIGMA_MATH_COMPILATION', ],
  link_with: [
    libligmabase,
  ],
  vs_module_defs: 'ligmamath.def',
  install: true,
  version: so_version,
)

install_headers(
  libligmamath_headers,
  subdir: ligma_api_name / 'libligmamath',
)
