
libgimpcolor_sources = files(
  'gimpadaptivesupersample.c',
  'gimpbilinear.c',
  'gimpcairo.c',
  'gimpcmyk.c',
  'gimpcolormanaged.c',
  'gimpcolorprofile.c',
  'gimpcolorspace.c',
  'gimpcolortransform.c',
  'gimphsl.c',
  'gimphsv.c',
  'gimppixbuf.c',
  'gimprgb-parse.c',
  'gimprgb.c',
)

libgimpcolor_headers_introspectable = files(
  'gimpcolortypes.h',
  'gimpadaptivesupersample.h',
  'gimpbilinear.h',
  'gimpcairo.h',
  'gimpcmyk.h',
  'gimpcolormanaged.h',
  'gimpcolorprofile.h',
  'gimpcolorspace.h',
  'gimpcolortransform.h',
  'gimphsl.h',
  'gimphsv.h',
  'gimppixbuf.h',
  'gimprgb.h',
)

libgimpcolor_headers = [
  libgimpcolor_headers_introspectable,
  'gimpcolor.h',
]

libgimpcolor_introspectable = [
  libgimpcolor_sources,
  libgimpcolor_headers_introspectable,
]


libgimpcolor = library('gimpcolor-' + gimp_api_version,
  libgimpcolor_sources,
  include_directories: rootInclude,
  dependencies: [
    cairo, gdk_pixbuf, gegl, lcms, math,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibGimpColor"', '-DGIMP_COLOR_COMPILATION', ],
  link_with: [ libgimpbase, ],
  vs_module_defs: 'gimpcolor.def',
  install: true,
  version: so_version,
)

install_headers(
  libgimpcolor_headers,
  subdir: gimp_api_name / 'libgimpcolor',
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
  c_args: '-DG_LOG_DOMAIN="LibGimpColor"',
  link_with: [ libgimpbase, libgimpcolor, ],
  install: false,
)
