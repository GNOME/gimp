libapplayermodes_sources = [
  'gimp-layer-modes.c',
  'gimpoperationantierase.c',
  'gimpoperationbehind.c',
  'gimpoperationdissolve.c',
  'gimpoperationerase.c',
  'gimpoperationlayermode-blend.c',
  'gimpoperationlayermode-composite-sse2.c',
  'gimpoperationlayermode-composite.c',
  'gimpoperationlayermode.c',
  'gimpoperationmerge.c',
  'gimpoperationnormal-sse2.c',
  'gimpoperationnormal-sse4.c',
  'gimpoperationnormal.c',
  'gimpoperationpassthrough.c',
  'gimpoperationreplace.c',
  'gimpoperationsplit.c',
]

libapplayermodes = static_library('applayermodes',
  libapplayermodes_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Layer-Modes"',
  dependencies: [
    cairo, gegl, gdk_pixbuf,
  ],
)
