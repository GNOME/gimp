
apptextenums = custom_target('text-enums.c',
  input : [ 'text-enums.h', ],
  output: [ 'text-enums.c', ],
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

libapptext_sources = [
  'gimpfont.c',
  'gimpfontfactory.c',
  'gimptext-compat.c',
  'gimptext-parasite.c',
  'gimptext-vectors.c',
  'gimptext-xlfd.c',
  'gimptext.c',
  'gimptextlayer-transform.c',
  'gimptextlayer-xcf.c',
  'gimptextlayer.c',
  'gimptextlayout-render.c',
  'gimptextlayout.c',
  'gimptextundo.c',
  apptextenums,

  appcoremarshal,
]


libapptext = static_library('apptext',
  libapptext_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Text"',
  dependencies: [
    gegl, gdk_pixbuf, harfbuzz, pangocairo, pangoft2,
  ],
)
