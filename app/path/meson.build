libapppath_sources = [
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
  'gimpvectorlayer.c',
  'gimpvectorlayeroptions.c',
  'gimpvectorlayerundo.c',
]

libapppath = static_library('apppath',
  libapppath_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Path"',
  dependencies: [
    cairo, gegl, gdk_pixbuf, gexiv2
  ],
)
