subdir('gfig-examples')
subdir('images')

plugin_name = 'gfig'

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
    'gfig-arc.c',
    'gfig-bezier.c',
    'gfig-circle.c',
    'gfig-dialog.c',
    'gfig-dobject.c',
    'gfig-ellipse.c',
    'gfig-grid.c',
    'gfig-line.c',
    'gfig-poly.c',
    'gfig-preview.c',
    'gfig-rectangle.c',
    'gfig-spiral.c',
    'gfig-star.c',
    'gfig-stock.c',
    'gfig-style.c',
    'gfig.c',
    stock_pixbufs_h,
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
  install: true,
  install_dir: join_paths(gimpplugindir, 'plug-ins'),
)
