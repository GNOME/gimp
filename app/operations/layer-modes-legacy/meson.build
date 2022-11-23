libapplayermodeslegacy_sources = [
  'ligmaoperationadditionlegacy.c',
  'ligmaoperationburnlegacy.c',
  'ligmaoperationdarkenonlylegacy.c',
  'ligmaoperationdifferencelegacy.c',
  'ligmaoperationdividelegacy.c',
  'ligmaoperationdodgelegacy.c',
  'ligmaoperationgrainextractlegacy.c',
  'ligmaoperationgrainmergelegacy.c',
  'ligmaoperationhardlightlegacy.c',
  'ligmaoperationhslcolorlegacy.c',
  'ligmaoperationhsvhuelegacy.c',
  'ligmaoperationhsvsaturationlegacy.c',
  'ligmaoperationhsvvaluelegacy.c',
  'ligmaoperationlightenonlylegacy.c',
  'ligmaoperationmultiplylegacy.c',
  'ligmaoperationscreenlegacy.c',
  'ligmaoperationsoftlightlegacy.c',
  'ligmaoperationsubtractlegacy.c',
]

libapplayermodeslegacy = static_library('applayermodeslegacy',
  libapplayermodeslegacy_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Ligma-Layer-Modes-Legacy"',
  dependencies: [
    cairo, gegl, gdk_pixbuf,
  ],
)
