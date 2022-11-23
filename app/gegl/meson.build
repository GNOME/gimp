stamp_gegl_enums = custom_target('stamp-ligma-gegl-enums.h',
  input : [
    files(
      'ligma-gegl-enums.h'
    ),
  ],
  output: [ 'stamp-ligma-gegl-enums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'ligma-gegl-',
    '#include <gio/gio.h>\n'              +
    '#include "libligmabase/ligmabase.h"\n' +
    '#include "core/core-enums.h"\n',
    '#include "ligma-intl.h"'
  ],
  build_by_default: true
)

libappgegl_loops = simd.check('ligma-gegl-loops-simd',
  sse2: 'ligma-gegl-loops-sse2.c',
  compiler: cc,
  include_directories: [ rootInclude, rootAppInclude, ],
  dependencies: [
    cairo,
    gegl,
    gdk_pixbuf,
  ],
)

libappgegl_sources = [
  'ligma-babl-compat.c',
  'ligma-babl.c',
  'ligma-gegl-apply-operation.c',
  'ligma-gegl-loops.cc',
  'ligma-gegl-mask-combine.cc',
  'ligma-gegl-mask.c',
  'ligma-gegl-nodes.c',
  'ligma-gegl-tile-compat.c',
  'ligma-gegl-utils.c',
  'ligma-gegl.c',
  'ligmaapplicator.c',
  'ligmatilehandlervalidate.c',

  'ligma-gegl-enums.c',
  stamp_gegl_enums
]

libappgegl = static_library('appgegl',
  libappgegl_sources,
  link_with: libappgegl_loops[0],
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Ligma-GEGL"',
  dependencies: [
    cairo,
    gegl,
    gdk_pixbuf,
  ],
)
