libapplayermodes_composite = simd.check('ligmaoperationlayermode-composite-simd',
  sse2: 'ligmaoperationlayermode-composite-sse2.c',
  compiler: cc,
  include_directories: [ rootInclude, rootAppInclude, ],
  dependencies: [
    cairo,
    gegl,
    gdk_pixbuf,
  ],
)

libapplayermodes_normal = simd.check('ligmaoperationnormal-simd',
  sse2: 'ligmaoperationnormal-sse2.c',
  sse41: 'ligmaoperationnormal-sse4.c',
  compiler: cc,
  include_directories: [ rootInclude, rootAppInclude, ],
  dependencies: [
    cairo,
    gegl,
    gdk_pixbuf,
  ],
)

libapplayermodes_sources = files(
  'ligma-layer-modes.c',
  'ligmaoperationantierase.c',
  'ligmaoperationbehind.c',
  'ligmaoperationdissolve.c',
  'ligmaoperationerase.c',
  'ligmaoperationlayermode-blend.c',
  'ligmaoperationlayermode-composite.c',
  'ligmaoperationlayermode.c',
  'ligmaoperationmerge.c',
  'ligmaoperationnormal.c',
  'ligmaoperationpassthrough.c',
  'ligmaoperationreplace.c',
  'ligmaoperationsplit.c',
)

libapplayermodes = static_library('applayermodes',
  libapplayermodes_sources,
  link_with: [
    libapplayermodes_composite[0],
    libapplayermodes_normal[0],
  ],
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Ligma-Layer-Modes"',
  dependencies: [
    cairo,
    gegl,
    gdk_pixbuf,
  ],
)
