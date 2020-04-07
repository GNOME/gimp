
appoperationsenums = custom_target('operations-enums.c',
  input : [ 'operations-enums.h', ],
  output: [ 'operations-enums.c', ],
  command: [
    gimp_mkenums,
    '--fhead','#include "config.h"\n'
            + '#include <gio/gio.h>\n'
            + '#include "libgimpbase/gimpbase.h"\n'
            + '#include "@basename@"\n'
            + '#include "gimp-intl.h"\n',
    app_mkenums_custom_target_commonargs,
    '@INPUT@',
  ],
  capture: true,
)


libappoperations_sources = [
  'gimp-operation-config.c',
  'gimp-operations.c',
  'gimpbrightnesscontrastconfig.c',
  'gimpcageconfig.c',
  'gimpcolorbalanceconfig.c',
  'gimpcurvesconfig.c',
  'gimphuesaturationconfig.c',
  'gimplevelsconfig.c',
  'gimpoperationborder.c',
  'gimpoperationbrightnesscontrast.c',
  'gimpoperationbuffersourcevalidate.c',
  'gimpoperationcagecoefcalc.c',
  'gimpoperationcagetransform.c',
  'gimpoperationcolorbalance.c',
  'gimpoperationcolorize.c',
  'gimpoperationcomposecrop.c',
  'gimpoperationcurves.c',
  'gimpoperationdesaturate.c',
  'gimpoperationequalize.c',
  'gimpoperationfillsource.c',
  'gimpoperationflood.c',
  'gimpoperationgradient.c',
  'gimpoperationgrow.c',
  'gimpoperationhistogramsink.c',
  'gimpoperationhuesaturation.c',
  'gimpoperationlevels.c',
  'gimpoperationmaskcomponents.cc',
  'gimpoperationoffset.c',
  'gimpoperationpointfilter.c',
  'gimpoperationposterize.c',
  'gimpoperationprofiletransform.c',
  'gimpoperationscalarmultiply.c',
  'gimpoperationsemiflatten.c',
  'gimpoperationsetalpha.c',
  'gimpoperationsettings.c',
  'gimpoperationshrink.c',
  'gimpoperationthreshold.c',
  'gimpoperationthresholdalpha.c',
  appoperationsenums
]

libappoperations = static_library('appoperations',
  libappoperations_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Operations"',
  dependencies: [
    cairo, gegl, gdk_pixbuf,
  ],
)

subdir('layer-modes')
subdir('layer-modes-legacy')
subdir('tests')
