
gimpconfigenums = custom_target('gimpconfigenums.c',
  input : [ 'gimpconfigenums.h', ],
  output: [ 'gimpconfigenums.c', ],
  command: [
    gimp_mkenums,
    '--fhead','#include "config.h"\n'
            + '#include <gio/gio.h>\n'
            + '#include "libgimpbase/gimpbase.h"\n'
            + '#include "@INPUT@"\n'
            + '#include "libgimp/libgimp-intl.h"\n',
    gimp_mkenums_custom_target_commonargs,
  ],
  capture: true,
)


libgimpconfig_sources = [
  'gimpcolorconfig.c',
  'gimpconfig-deserialize.c',
  'gimpconfig-error.c',
  'gimpconfig-iface.c',
  'gimpconfig-path.c',
  'gimpconfig-serialize.c',
  'gimpconfig-utils.c',
  'gimpconfigwriter.c',
  'gimpscanner.c',
  gimpconfigenums
]

libgimpconfig_headers = [
  'gimpcolorconfig.h',
  'gimpconfig-deserialize.h',
  'gimpconfig-error.h',
  'gimpconfig-iface.h',
  'gimpconfig-params.h',
  'gimpconfig-path.h',
  'gimpconfig-serialize.h',
  'gimpconfig-utils.h',
  'gimpconfig.h',
  'gimpconfigenums.h',
  'gimpconfigtypes.h',
  'gimpconfigwriter.h',
  'gimpscanner.h',
]

libgimpconfig = library('gimpconfig-' + api_version,
  libgimpconfig_sources,
  include_directories: rootInclude,
  dependencies: [
    cairo, gdk_pixbuf, gegl, gio, gio_specific,
  ],
  c_args:     [
    '-DG_LOG_DOMAIN="LibGimpConfig"',
    '-DGIMP_CONFIG_COMPILATION',
  ],
  link_with:  [
    libgimpbase,
    libgimpcolor,
    libgimpmath,
  ],
  vs_module_defs: 'gimpconfig.def',
  install: true,
  version: so_version,
)

install_headers(
  libgimpconfig_headers,
  subdir: join_paths(gimp_api_name, 'libgimpconfig'),
)
