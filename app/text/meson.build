stamp_text_enums = custom_target('stamp-text-enums.h',
  input : [
    files(
      'text-enums.h'
    ),
  ],
  output: [ 'stamp-text-enums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'text-',
    '#include <gio/gio.h>\n'              +
    '#include "libgimpbase/gimpbase.h"\n',
    '#include "gimp-intl.h"'
  ],
  build_by_default: true
)

libapptext_sources = [
  'gimpfont.c',
  'gimpfontfactory.c',
  'gimptext-compat.c',
  'gimptext-parasite.c',
  'gimptext-path.c',
  'gimptext-xlfd.c',
  'gimptext.c',
  'gimptextlayer-transform.c',
  'gimptextlayer-xcf.c',
  'gimptextlayer.c',
  'gimptextlayout-render.c',
  'gimptextlayout.c',
  'gimptextundo.c',

  'text-enums.c',
  stamp_text_enums
]


libapptext = static_library('apptext',
  libapptext_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Text"',
  dependencies: [
    gegl, gdk_pixbuf, gexiv2, harfbuzz, pangocairo, pangoft2,
  ],
)
