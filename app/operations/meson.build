stamp_operations_enums = custom_target('stamp-operations-enums.h',
  input : [
    files(
      'operations-enums.h'
    ),
  ],
  output: [ 'stamp-operations-enums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'operations-',
    '#include <gio/gio.h>\n'              +
    '#include "libligmabase/ligmabase.h"\n',
    '#include "ligma-intl.h"'
  ],
  build_by_default: true
)

libappoperations_sources = [
  'ligma-operation-config.c',
  'ligma-operations.c',
  'ligmabrightnesscontrastconfig.c',
  'ligmacageconfig.c',
  'ligmacolorbalanceconfig.c',
  'ligmacurvesconfig.c',
  'ligmahuesaturationconfig.c',
  'ligmalevelsconfig.c',
  'ligmaoperationborder.c',
  'ligmaoperationbrightnesscontrast.c',
  'ligmaoperationbuffersourcevalidate.c',
  'ligmaoperationcagecoefcalc.c',
  'ligmaoperationcagetransform.c',
  'ligmaoperationcolorbalance.c',
  'ligmaoperationcolorize.c',
  'ligmaoperationcomposecrop.c',
  'ligmaoperationcurves.c',
  'ligmaoperationdesaturate.c',
  'ligmaoperationequalize.c',
  'ligmaoperationfillsource.c',
  'ligmaoperationflood.c',
  'ligmaoperationgradient.c',
  'ligmaoperationgrow.c',
  'ligmaoperationhistogramsink.c',
  'ligmaoperationhuesaturation.c',
  'ligmaoperationlevels.c',
  'ligmaoperationmaskcomponents.cc',
  'ligmaoperationoffset.c',
  'ligmaoperationpointfilter.c',
  'ligmaoperationposterize.c',
  'ligmaoperationprofiletransform.c',
  'ligmaoperationscalarmultiply.c',
  'ligmaoperationsemiflatten.c',
  'ligmaoperationsetalpha.c',
  'ligmaoperationsettings.c',
  'ligmaoperationshrink.c',
  'ligmaoperationthreshold.c',
  'ligmaoperationthresholdalpha.c',

  'operations-enums.c',
  stamp_operations_enums,
]

libappoperations = static_library('appoperations',
  libappoperations_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Ligma-Operations"',
  dependencies: [
    cairo, gegl, gdk_pixbuf,
  ],
)

subdir('layer-modes')
subdir('layer-modes-legacy')
subdir('tests')
