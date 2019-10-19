
appgeglenums = custom_target('gimp-gegl-enums.c',
  input : [ 'gimp-gegl-enums.h', ],
  output: [ 'gimp-gegl-enums.c', ],
  command: [
    gimp_mkenums,
    '--fhead','#include "config.h"\n'
            + '#include <gio/gio.h>\n'
            + '#include "libgimpbase/gimpbase.h"\n'
            + '#include "core/core-enums.h"\n'
            + '#include "@basename@"\n'
            + '#include "gimp-intl.h"\n',
    app_mkenums_custom_target_commonargs,
    '@INPUT@',
  ],
  capture: true,
)

libappgegl_sources = [
  'gimp-babl-compat.c',
  'gimp-babl.c',
  'gimp-gegl-apply-operation.c',
  'gimp-gegl-loops-sse2.c',
  'gimp-gegl-loops.cc',
  'gimp-gegl-mask-combine.cc',
  'gimp-gegl-mask.c',
  'gimp-gegl-nodes.c',
  'gimp-gegl-tile-compat.c',
  'gimp-gegl-utils.c',
  'gimp-gegl.c',
  'gimpapplicator.c',
  'gimptilehandlervalidate.c',
  appgeglenums,
]

libappgegl = static_library('appgegl',
  libappgegl_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-GEGL"',
  dependencies: [
    cairo, gegl, gdk_pixbuf,
  ],
)
