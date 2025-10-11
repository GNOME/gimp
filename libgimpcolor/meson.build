libgimpcolor_sources = files(
  'gimpadaptivesupersample.c',
  'gimpbilinear.c',
  'gimpcairo.c',
  'gimpcolor.c',
  'gimpcolor-parse.c',
  'gimpcolormanaged.c',
  'gimpcolorprofile.c',
  'gimpcolortransform.c',
  'gimppixbuf.c',
)

libgimpcolor_headers_introspectable = files(
  'gimpcolortypes.h',
  'gimpadaptivesupersample.h',
  'gimpbilinear.h',
  'gimpcairo.h',
  'gimpcolormanaged.h',
  'gimpcolorprofile.h',
  'gimpcolortransform.h',
  'gimppixbuf.h',

  'gimpcolor.h',
)

libgimpcolor_headers = [
  libgimpcolor_headers_introspectable,
]

libgimpcolor_introspectable = [
  libgimpcolor_sources,
  libgimpcolor_headers_introspectable,
]

libgimpcolor = library('gimpcolor-' + gimp_api_version,
  libgimpcolor_sources,
  include_directories: rootInclude,
  dependencies: [
    cairo, gdk_pixbuf, gegl, gexiv2, lcms, math,
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
