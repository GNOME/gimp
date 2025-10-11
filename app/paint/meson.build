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
    '#include "libgimpbase/gimpbase.h"\n',
    '#include "gimp-intl.h"'
  ],
  build_by_default: true
)

libapppaint_sources = [
  'gimp-paint.c',
  'gimpairbrush.c',
  'gimpairbrushoptions.c',
  'gimpbrushcore-loops.cc',
  'gimpbrushcore.c',
  'gimpclone.c',
  'gimpcloneoptions.c',
  'gimpconvolve.c',
  'gimpconvolveoptions.c',
  'gimpdodgeburn.c',
  'gimpdodgeburnoptions.c',
  'gimperaser.c',
  'gimperaseroptions.c',
  'gimpheal.c',
  'gimpink-blob.c',
  'gimpink.c',
  'gimpinkoptions.c',
  'gimpinkundo.c',
  'gimpmybrushcore.c',
  'gimpmybrushoptions.c',
  'gimpmybrushsurface.c',
  'gimppaintbrush.c',
  'gimppaintcore-loops.cc',
  'gimppaintcore-stroke.c',
  'gimppaintcore.c',
  'gimppaintcoreundo.c',
  'gimppaintoptions.c',
  'gimppencil.c',
  'gimppenciloptions.c',
  'gimpperspectiveclone.c',
  'gimpperspectivecloneoptions.c',
  'gimpsmudge.c',
  'gimpsmudgeoptions.c',
  'gimpsourcecore.c',
  'gimpsourceoptions.c',

  'paint-enums.c',
  stamp_paint_enums,
]

libapppaint = static_library('apppaint',
  libapppaint_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Paint"',
  dependencies: [
    cairo, gegl, gdk_pixbuf, gexiv2, libmypaint,
  ],
)
