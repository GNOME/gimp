stamp_config_enums = custom_target('stamp-config-enums.h',
  input : [
    files(
      'config-enums.h'
    ),
  ],
  output: [ 'stamp-config-enums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'config-',
    '#include <gio/gio.h>\n'              +
    '#include "libligmabase/ligmabase.h"\n',
    '#include "ligma-intl.h"'
  ],
  build_by_default: true
)

libappconfig_sources = [
  'ligmaconfig-dump.c',
  'ligmaconfig-file.c',
  'ligmaconfig-utils.c',
  'ligmacoreconfig.c',
  'ligmadialogconfig.c',
  'ligmadisplayconfig.c',
  'ligmadisplayoptions.c',
  'ligmaearlyrc.c',
  'ligmageglconfig.c',
  'ligmaguiconfig.c',
  'ligmapluginconfig.c',
  'ligmarc-deserialize.c',
  'ligmarc-serialize.c',
  'ligmarc-unknown.c',
  'ligmarc.c',
  'ligmaxmlparser.c',

  'config-enums.c',
  stamp_config_enums,
]

libappconfig = static_library('appconfig',
  libappconfig_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Ligma-Config"',
  dependencies: [
    cairo, gegl, gdk_pixbuf, gio, gio_specific, libmypaint,
  ],
)

test('app-config',
  executable('test-config',
    [ 'test-config.c', app_debug_files, ],

    dependencies: [
      appstream_glib,
      libapp_dep,
    ],
    link_with: [
      libligmabase,
      libligmaconfig,
      libligmacolor,
      libligmamath,
      libligmamodule,
      libligmathumb,

      libappconfig,
    ],
    install: false,
  ),
  suite: 'app'
)
