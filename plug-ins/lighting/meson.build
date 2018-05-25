subdir('images')

plugin_name = 'lighting'

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
    'lighting-apply.c',
    'lighting-image.c',
    'lighting-main.c',
    'lighting-preview.c',
    'lighting-shade.c',
    'lighting-stock.c',
    'lighting-ui.c',
    stock_pixbufs_h,
    rc_file,
  ],
  include_directories: [ rootInclude, include_directories('.'), ],
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
  install: true,
  install_dir: join_paths(gimpplugindir, 'plug-ins', plugin_name),
)
