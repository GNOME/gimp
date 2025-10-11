stamp_gegl_enums = custom_target('stamp-gimp-gegl-enums.h',
  input : [
    files(
      'gimp-gegl-enums.h'
    ),
  ],
  output: [ 'stamp-gimp-gegl-enums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'gimp-gegl-',
    '#include <gio/gio.h>\n'              +
    '#include "libgimpbase/gimpbase.h"\n' +
    '#include "core/core-enums.h"\n',
    '#include "gimp-intl.h"'
  ],
  build_by_default: true
)

libappgegl_loops = simd.check('gimp-gegl-loops-simd',
  sse2: 'gimp-gegl-loops-sse2.c',
  compiler: cc,
  include_directories: [ rootInclude, rootAppInclude, ],
  dependencies: [
    cairo,
    gegl,
    gdk_pixbuf,
  ],
)

libappgegl_sources = [
  'gimp-babl-compat.c',
  'gimp-babl.c',
  'gimp-gegl-apply-operation.c',
  'gimp-gegl-loops.cc',
  'gimp-gegl-mask-combine.cc',
  'gimp-gegl-mask.c',
  'gimp-gegl-nodes.c',
  'gimp-gegl-tile-compat.c',
  'gimp-gegl-utils.c',
  'gimp-gegl.c',
  'gimpapplicator.c',
  'gimptilehandlervalidate.c',

  'gimp-gegl-enums.c',
  stamp_gegl_enums
]

libappgegl = static_library('appgegl',
  libappgegl_sources,
  link_with: libappgegl_loops[0],
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-GEGL"',
  dependencies: [
    cairo,
    gegl,
    gdk_pixbuf,
    gexiv2
  ],
)
