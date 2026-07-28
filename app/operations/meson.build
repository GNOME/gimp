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
    '#include "libgimpbase/gimpbase.h"\n',
    '#include "gimp-intl.h"'
  ],
  build_by_default: true
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

  'operations-enums.c',
  stamp_operations_enums,
]

libappoperations = static_library('appoperations',
  libappoperations_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Operations"',
  dependencies: [
    cairo, gegl, gdk_pixbuf, gexiv2
  ],
)

subdir('layer-modes')
subdir('layer-modes-legacy')
subdir('tests')
