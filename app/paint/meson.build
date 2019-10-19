
apppaintenums = custom_target('paint-enums.c',
  input : [ 'paint-enums.h', ],
  output: [ 'paint-enums.c', ],
  command: [
    gimp_mkenums,
    '--fhead','#include "config.h"\n'
            + '#include <gio/gio.h>\n'
            + '#include "libgimpbase/gimpbase.h"\n'
            + '#include "@basename@"\n'
            + '#include "gimp-intl.h"\n',
    app_mkenums_custom_target_commonargs,
    '@INPUT@',
  ],
  capture: true,
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
  apppaintenums,

  appcoremarshal,
]

libapppaint = static_library('apppaint',
  libapppaint_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Paint"',
  dependencies: [
    cairo, gegl, gdk_pixbuf, libmypaint,
  ],
)
