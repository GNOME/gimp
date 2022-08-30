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
    '#include "libgimpbase/gimpbase.h"\n',
    '#include "gimp-intl.h"'
  ],
  build_by_default: true
)

libappconfig_sources = [
  'gimpconfig-dump.c',
  'gimpconfig-file.c',
  'gimpconfig-utils.c',
  'gimpcoreconfig.c',
  'gimpdialogconfig.c',
  'gimpdisplayconfig.c',
  'gimpdisplayoptions.c',
  'gimpearlyrc.c',
  'gimpgeglconfig.c',
  'gimpguiconfig.c',
  'gimppluginconfig.c',
  'gimprc-deserialize.c',
  'gimprc-serialize.c',
  'gimprc-unknown.c',
  'gimprc.c',
  'gimpxmlparser.c',

  'config-enums.c',
  stamp_config_enums,
]

libappconfig = static_library('appconfig',
  libappconfig_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Config"',
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
      libgimpbase,
      libgimpconfig,
      libgimpcolor,
      libgimpmath,
      libgimpmodule,
      libgimpthumb,

      libappconfig,
    ],
    install: false,
  ),
  suite: 'app'
)
