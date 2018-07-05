plugin_name = 'help'

rc_file = platform_win32 ? windows.compile_resources(
  gimp_plugins_rc,
  args: [
    '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plugin_name+'.exe'),
    '--define', 'INTERNALNAME_STR="@0@"'    .format(plugin_name),
    '--define', 'TOP_SRCDIR="@0@"'          .format(meson.source_root()),
  ],
  include_directories: [
    rootInclude, appInclude,
  ],
) : []

help_plugin_lib = static_library('help_plugin',
  [
    # 'gimp-help-lookup.c',
    'gimphelp.c',
    'gimphelpdomain.c',
    'gimphelpitem.c',
    'gimphelplocale.c',
    'gimphelpprogress.c',
    rc_file,
  ],
  include_directories: [ rootInclude, ],
  dependencies: [
    gtk3, gegl, cairo, gio, gdk_pixbuf,
  ],
  install: false,
)

executable(plugin_name,
  'help.c',
  include_directories: [ rootInclude, ],
  dependencies: [
    gtk3, gegl, cairo, gio, gdk_pixbuf,
  ],
  link_with: [
    help_plugin_lib,
    libgimp,
    libgimpbase,
    libgimpcolor,
    libgimpconfig,
    libgimpmath,
    libgimpui,
    libgimpwidgets,
  ],
  install: true,
  install_dir: join_paths(gimpplugindir, 'plug-ins', plugin_name),
)
