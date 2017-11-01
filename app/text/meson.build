
apptextenums = custom_target('text-enums.c',
  input : [ 'text-enums.h', ],
  output: [ 'text-enums.c', ],
  command: app_mkenums_custom_target_command,
  capture: true,
)

libapptext_sources = [
  'gimp-fonts.c',
  'gimpfont.c',
  'gimpfontlist.c',
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
  c_args: [
    '-DG_LOG_DOMAIN="Gimp-Text"',
  ],
  dependencies: [
    gegl, gdk_pixbuf, harfbuzz, pangocairo, pangoft2,
  ],
)
