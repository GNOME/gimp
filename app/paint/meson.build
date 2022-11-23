stamp_paint_enums = custom_target('stamp-paint-enums.h',
  input : [
    files(
      'paint-enums.h'
    ),
  ],
  output: [ 'stamp-paint-enums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'paint-',
    '#include <gio/gio.h>\n'              +
    '#include "libligmabase/ligmabase.h"\n',
    '#include "ligma-intl.h"'
  ],
  build_by_default: true
)

libapppaint_sources = [
  'ligma-paint.c',
  'ligmaairbrush.c',
  'ligmaairbrushoptions.c',
  'ligmabrushcore-loops.cc',
  'ligmabrushcore.c',
  'ligmaclone.c',
  'ligmacloneoptions.c',
  'ligmaconvolve.c',
  'ligmaconvolveoptions.c',
  'ligmadodgeburn.c',
  'ligmadodgeburnoptions.c',
  'ligmaeraser.c',
  'ligmaeraseroptions.c',
  'ligmaheal.c',
  'ligmaink-blob.c',
  'ligmaink.c',
  'ligmainkoptions.c',
  'ligmainkundo.c',
  'ligmamybrushcore.c',
  'ligmamybrushoptions.c',
  'ligmamybrushsurface.c',
  'ligmapaintbrush.c',
  'ligmapaintcore-loops.cc',
  'ligmapaintcore-stroke.c',
  'ligmapaintcore.c',
  'ligmapaintcoreundo.c',
  'ligmapaintoptions.c',
  'ligmapencil.c',
  'ligmapenciloptions.c',
  'ligmaperspectiveclone.c',
  'ligmaperspectivecloneoptions.c',
  'ligmasmudge.c',
  'ligmasmudgeoptions.c',
  'ligmasourcecore.c',
  'ligmasourceoptions.c',

  'paint-enums.c',
  stamp_paint_enums,
]

libapppaint = static_library('apppaint',
  libapppaint_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Ligma-Paint"',
  dependencies: [
    cairo, gegl, gdk_pixbuf, libmypaint,
  ],
)
