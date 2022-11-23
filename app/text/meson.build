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
    '#include "libligmabase/ligmabase.h"\n',
    '#include "ligma-intl.h"'
  ],
  build_by_default: true
)

libapptext_sources = [
  'ligmafont.c',
  'ligmafontfactory.c',
  'ligmatext-compat.c',
  'ligmatext-parasite.c',
  'ligmatext-vectors.c',
  'ligmatext-xlfd.c',
  'ligmatext.c',
  'ligmatextlayer-transform.c',
  'ligmatextlayer-xcf.c',
  'ligmatextlayer.c',
  'ligmatextlayout-render.c',
  'ligmatextlayout.c',
  'ligmatextundo.c',

  'text-enums.c',
  stamp_text_enums
]


libapptext = static_library('apptext',
  libapptext_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Ligma-Text"',
  dependencies: [
    gegl, gdk_pixbuf, harfbuzz, pangocairo, pangoft2,
  ],
)
