subdir('Brushes')
subdir('Paper')
subdir('Presets')

plugin_name = 'gimpressionist'

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

executable(plugin_name,
  [
    'brush.c',
    'color.c',
    'general.c',
    'gimp.c',
    'gimpressionist.c',
    'globals.c',
    'orientation.c',
    'orientmap.c',
    'paper.c',
    'placement.c',
    'plasma.c',
    'ppmtool.c',
    'presets.c',
    'preview.c',
    'repaint.c',
    'size.c',
    'sizemap.c',
    'utils.c',
    rc_file,
  ],
  include_directories: [ rootInclude, ],
  dependencies: [
    gtk3, gegl, math,
  ],
  link_with: [
    libgimp,
    libgimpbase,
    libgimpcolor,
    libgimpconfig,
    libgimpmath,
    libgimpui,
    libgimpwidgets,
  ],
  c_args: [
    '-DDEFAULTPATH="' + join_paths('~/',  gimpdir,    'gimpressionist')
                + ':' + join_paths(prefix,gimpdatadir,'gimpressionist')
                + '"',
  ],
  install: true,
  install_dir: join_paths(gimpplugindir, 'plug-ins'),
)
