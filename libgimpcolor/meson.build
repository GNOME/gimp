
libligmacolor_sources = files(
  'ligmaadaptivesupersample.c',
  'ligmabilinear.c',
  'ligmacairo.c',
  'ligmacmyk.c',
  'ligmacolormanaged.c',
  'ligmacolorprofile.c',
  'ligmacolorspace.c',
  'ligmacolortransform.c',
  'ligmahsl.c',
  'ligmahsv.c',
  'ligmapixbuf.c',
  'ligmargb-parse.c',
  'ligmargb.c',
)

libligmacolor_headers_introspectable = files(
  'ligmacolortypes.h',
  'ligmaadaptivesupersample.h',
  'ligmabilinear.h',
  'ligmacairo.h',
  'ligmacmyk.h',
  'ligmacolormanaged.h',
  'ligmacolorprofile.h',
  'ligmacolorspace.h',
  'ligmacolortransform.h',
  'ligmahsl.h',
  'ligmahsv.h',
  'ligmapixbuf.h',
  'ligmargb.h',
)

libligmacolor_headers = [
  libligmacolor_headers_introspectable,
  'ligmacolor.h',
]

libligmacolor_introspectable = [
  libligmacolor_sources,
  libligmacolor_headers_introspectable,
]


libligmacolor = library('ligmacolor-' + ligma_api_version,
  libligmacolor_sources,
  include_directories: rootInclude,
  dependencies: [
    cairo, gdk_pixbuf, gegl, lcms, math,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibLigmaColor"', '-DLIGMA_COLOR_COMPILATION', ],
  link_with: [ libligmabase, ],
  vs_module_defs: 'ligmacolor.def',
  install: true,
  version: so_version,
)

install_headers(
  libligmacolor_headers,
  subdir: ligma_api_name / 'libligmacolor',
)


# Test program, not installed
executable('test-color-parser',
  'test-color-parser.c',
  include_directories: rootInclude,
  dependencies: [
    cairo, gdk_pixbuf, gegl, lcms, math,
    babl,
    # glib,
  ],
  c_args: '-DG_LOG_DOMAIN="LibLigmaColor"',
  link_with: [ libligmabase, libligmacolor, ],
  install: false,
)
