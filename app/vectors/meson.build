libappvectors_sources = [
  'gimpanchor.c',
  'gimpbezierstroke.c',
  'gimppath.c',
  'gimppath-compat.c',
  'gimppath-export.c',
  'gimppath-import.c',
  'gimppath-preview.c',
  'gimppath-warp.c',
  'gimppathmodundo.c',
  'gimppathpropundo.c',
  'gimppathundo.c',
  'gimpstroke-new.c',
  'gimpstroke.c',
]

libappvectors = static_library('appvectors',
  libappvectors_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Vectors"',
  dependencies: [
    cairo, gegl, gdk_pixbuf,
  ],
)
