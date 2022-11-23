
stamp_config_enums = custom_target('stamp-ligmaconfigenums.h',
  input : [
    files(
      'ligmaconfigenums.h'
    ),
  ],
  output: [ 'stamp-ligmaconfigenums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'ligmaconfig',
    '#include <gio/gio.h>\n' +
    '#include "libligmabase/ligmabase.h"\n',
    '#include "libligma/libligma-intl.h"',
    libligma_mkenums_dtails
  ],
  build_by_default: true
)

libligmaconfig_sources_introspectable = files(
  'ligmacolorconfig.c',
  'ligmaconfig-deserialize.c',
  'ligmaconfig-error.c',
  'ligmaconfig-iface.c',
  'ligmaconfig-params.c',
  'ligmaconfig-path.c',
  'ligmaconfig-register.c',
  'ligmaconfig-serialize.c',
  'ligmaconfig-utils.c',
  'ligmaconfigwriter.c',
  'ligmascanner.c',
)

libligmaconfig_sources = [
  libligmaconfig_sources_introspectable,

  'ligmaconfigenums.c',
  stamp_config_enums
]

libligmaconfig_headers_introspectable = files(
  'ligmacolorconfig.h',
  'ligmaconfig-deserialize.h',
  'ligmaconfig-error.h',
  'ligmaconfig-iface.h',
  'ligmaconfig-params.h',
  'ligmaconfig-path.h',
  'ligmaconfig-register.h',
  'ligmaconfig-serialize.h',
  'ligmaconfig-utils.h',
  'ligmaconfigenums.h',
  'ligmaconfigtypes.h',
  'ligmaconfigwriter.h',
  'ligmascanner.h',
)

libligmaconfig_headers = [
  libligmaconfig_headers_introspectable,
  'ligmaconfig.h',
]

libligmaconfig_introspectable = [
  libligmaconfig_headers_introspectable,
  libligmaconfig_sources_introspectable,
]

libligmaconfig = library('ligmaconfig-' + ligma_api_version,
  libligmaconfig_sources,
  include_directories: rootInclude,
  dependencies: [
    cairo, gdk_pixbuf, gegl, gio, gio_specific,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibLigmaConfig"', '-DLIGMA_CONFIG_COMPILATION', ],
  link_with: [
    libligmabase,
    libligmacolor,
    libligmamath,
  ],
  vs_module_defs: 'ligmaconfig.def',
  install: true,
  version: so_version,
)

install_headers(
  libligmaconfig_headers,
  subdir: ligma_api_name / 'libligmaconfig',
)
