
stamp_config_enums = custom_target('stamp-gimpconfigenums.h',
  input : [
    files(
      'gimpconfigenums.h'
    ),
  ],
  output: [ 'stamp-gimpconfigenums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'gimpconfig',
    '#include <gio/gio.h>\n' +
    '#include "libgimpbase/gimpbase.h"\n',
    '#include "libgimp/libgimp-intl.h"',
    libgimp_mkenums_dtails
  ],
  build_by_default: true
)

libgimpconfig_sources_introspectable = files(
  'gimpcolorconfig.c',
  'gimpconfig-array.c',
  'gimpconfig-deserialize.c',
  'gimpconfig-error.c',
  'gimpconfig-iface.c',
  'gimpconfig-params.c',
  'gimpconfig-path.c',
  'gimpconfig-register.c',
  'gimpconfig-serialize.c',
  'gimpconfig-utils.c',
  'gimpconfigwriter.c',
  'gimpscanner.c',
)

libgimpconfig_sources = [
  libgimpconfig_sources_introspectable,

  'gimpconfigenums.c',
  stamp_config_enums
]

libgimpconfig_headers_introspectable = files(
  'gimpcolorconfig.h',
  'gimpconfig-array.h',
  'gimpconfig-deserialize.h',
  'gimpconfig-error.h',
  'gimpconfig-iface.h',
  'gimpconfig-params.h',
  'gimpconfig-path.h',
  'gimpconfig-register.h',
  'gimpconfig-serialize.h',
  'gimpconfig-utils.h',
  'gimpconfigenums.h',
  'gimpconfigtypes.h',
  'gimpconfigwriter.h',
  'gimpscanner.h',
)

libgimpconfig_headers = [
  libgimpconfig_headers_introspectable,
  'gimpconfig.h',
]

libgimpconfig_introspectable = [
  libgimpconfig_headers_introspectable,
  libgimpconfig_sources_introspectable,
]

libgimpconfig = library('gimpconfig-' + gimp_api_version,
  libgimpconfig_sources,
  include_directories: rootInclude,
  dependencies: [
    cairo, gdk_pixbuf, gegl, gio, gio_specific,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibGimpConfig"', '-DGIMP_CONFIG_COMPILATION', ],
  link_with: [
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
  subdir: gimp_api_name / 'libgimpconfig',
)
