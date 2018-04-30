libapplayermodeslegacy_sources = [
  'gimpoperationadditionlegacy.c',
  'gimpoperationburnlegacy.c',
  'gimpoperationdarkenonlylegacy.c',
  'gimpoperationdifferencelegacy.c',
  'gimpoperationdividelegacy.c',
  'gimpoperationdodgelegacy.c',
  'gimpoperationgrainextractlegacy.c',
  'gimpoperationgrainmergelegacy.c',
  'gimpoperationhardlightlegacy.c',
  'gimpoperationhslcolorlegacy.c',
  'gimpoperationhsvhuelegacy.c',
  'gimpoperationhsvsaturationlegacy.c',
  'gimpoperationhsvvaluelegacy.c',
  'gimpoperationlightenonlylegacy.c',
  'gimpoperationmultiplylegacy.c',
  'gimpoperationscreenlegacy.c',
  'gimpoperationsoftlightlegacy.c',
  'gimpoperationsubtractlegacy.c',
]

libapplayermodeslegacy = static_library('applayermodeslegacy',
  libapplayermodeslegacy_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Layer-Modes-Legacy"',
  dependencies: [
    cairo, gegl, gdk_pixbuf,
  ],
)
