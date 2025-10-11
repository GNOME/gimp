libapplayermodes_composite = simd.check('gimpoperationlayermode-composite-simd',
  sse2: 'gimpoperationlayermode-composite-sse2.c',
  compiler: cc,
  include_directories: [ rootInclude, rootAppInclude, ],
  dependencies: [
    cairo,
    gegl,
    gdk_pixbuf,
  ],
)

libapplayermodes_normal = simd.check('gimpoperationnormal-simd',
  sse2: 'gimpoperationnormal-sse2.c',
  sse41: 'gimpoperationnormal-sse4.c',
  compiler: cc,
  include_directories: [ rootInclude, rootAppInclude, ],
  dependencies: [
    cairo,
    gegl,
    gdk_pixbuf,
  ],
)

libapplayermodes_sources = files(
  'gimp-layer-modes.c',
  'gimpoperationantierase.c',
  'gimpoperationbehind.c',
  'gimpoperationdissolve.c',
  'gimpoperationerase.c',
  'gimpoperationlayermode-blend.c',
  'gimpoperationlayermode-composite.c',
  'gimpoperationlayermode.c',
  'gimpoperationmerge.c',
  'gimpoperationnormal.c',
  'gimpoperationpassthrough.c',
  'gimpoperationoverwrite.c',
  'gimpoperationreplace.c',
  'gimpoperationsplit.c',
)

libapplayermodes = static_library('applayermodes',
  libapplayermodes_sources,
  link_with: [
    libapplayermodes_composite[0],
    libapplayermodes_normal[0],
  ],
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Layer-Modes"',
  dependencies: [
    cairo,
    gegl,
    gdk_pixbuf,
    gexiv2
  ],
)
