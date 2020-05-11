
appconfigenums = custom_target('config-enums.c',
  input : [ 'config-enums.h', ],
  output: [ 'config-enums.c', ],
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

libappconfig_sources = [
  'gimpconfig-dump.c',
  'gimpconfig-file.c',
  'gimpconfig-utils.c',
  'gimpcoreconfig.c',
  'gimpdialogconfig.c',
  'gimpdisplayconfig.c',
  'gimpdisplayoptions.c',
  'gimpgeglconfig.c',
  'gimpguiconfig.c',
  'gimplangrc.c',
  'gimppluginconfig.c',
  'gimprc-deserialize.c',
  'gimprc-serialize.c',
  'gimprc-unknown.c',
  'gimprc.c',
  'gimpxmlparser.c',
  appconfigenums,
]

libappconfig = static_library('appconfig',
  libappconfig_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Gimp-Config"',
  dependencies: [
    cairo, gegl, gdk_pixbuf, gio, gio_specific, gtk3, libmypaint,
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
