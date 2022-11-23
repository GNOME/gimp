libappvectors_sources = [
  'ligmaanchor.c',
  'ligmabezierstroke.c',
  'ligmastroke-new.c',
  'ligmastroke.c',
  'ligmavectors-compat.c',
  'ligmavectors-export.c',
  'ligmavectors-import.c',
  'ligmavectors-preview.c',
  'ligmavectors-warp.c',
  'ligmavectors.c',
  'ligmavectorsmodundo.c',
  'ligmavectorspropundo.c',
  'ligmavectorsundo.c',
]

libappvectors = static_library('appvectors',
  libappvectors_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Ligma-Vectors"',
  dependencies: [
    cairo, gegl, gdk_pixbuf,
  ],
)
