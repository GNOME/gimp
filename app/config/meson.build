
appconfigenums = custom_target('config-enums.c',
  input : [ 'config-enums.h', ],
  output: [ 'config-enums.c', ],
  command: app_mkenums_custom_target_command,
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
  c_args: [
    '-DG_LOG_DOMAIN="Gimp-Config"',
    mypaint_args,
  ],
  dependencies: [
    cairo, gegl, gdk_pixbuf, gio, gio_specific, gtk2, libmypaint, mypaint_brushes,
  ],
)

test('app-config',
  executable('test-config',
    [ 'test-config.c', app_debug_files, ],
    include_directories: [ rootInclude, rootAppInclude, ],

    dependencies: [
      gegl, gio, gio_specific,
    ],
    link_with: [
      libgimpbase,
      libgimpconfig,
      libgimpcolor,
      libgimpmath,
      libgimpmodule,
      libgimpthumb,

      libappcore,
      libappconfig,
      libappfile,
      libappgegl,
      libappinternalprocs,
      libapplayermodes,
      libapplayermodeslegacy,
      libappoperations,
      libapppaint,
      libapppdb,
      libappplugin,
      libapptext,
      libappvectors,
      libappxcf,
    ],
    install: false,
  ),
  suite: 'app'
)
