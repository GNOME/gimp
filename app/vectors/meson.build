libappvectors_sources = [
  'gimpanchor.c',
  'gimpbezierstroke.c',
  'gimpstroke-new.c',
  'gimpstroke.c',
  'gimpvectors-compat.c',
  'gimpvectors-export.c',
  'gimpvectors-import.c',
  'gimpvectors-preview.c',
  'gimpvectors-warp.c',
  'gimpvectors.c',
  'gimpvectorsmodundo.c',
  'gimpvectorspropundo.c',
  'gimpvectorsundo.c',

  appcoremarshal,
]

libappvectors = static_library('appvectors',
  libappvectors_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Vectors"',
  dependencies: [
    cairo, gegl, gdk_pixbuf,
  ],
)
