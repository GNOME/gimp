libapppropgui_sources = [
  'gimppropgui-channel-mixer.c',
  'gimppropgui-color-balance.c',
  'gimppropgui-color-rotate.c',
  'gimppropgui-color-to-alpha.c',
  'gimppropgui-convolution-matrix.c',
  'gimppropgui-diffraction-patterns.c',
  'gimppropgui-eval.c',
  'gimppropgui-generic.c',
  'gimppropgui-hue-saturation.c',
  'gimppropgui-shadows-highlights.c',
  'gimppropgui-spiral.c',
  'gimppropgui-supernova.c',
  'gimppropgui-utils.c',
  'gimppropgui.c',
]

libapppropgui = static_library('apppropgui',
  libapppropgui_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: [
    '-DG_LOG_DOMAIN="Gimp-PropGUI"',
  ],
  dependencies: [
    gegl, gtk2,
  ],
)
