plugin_name = 'file-psd'

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
    'psd-image-res-load.c',
    'psd-layer-res-load.c',
    'psd-load.c',
    'psd-save.c',
    'psd-thumb-load.c',
    'psd-util.c',
    'psd.c',
    rc_file,
  ],
  include_directories: [ rootInclude, ],
  dependencies: [
    gtk3, gegl, libjpeg, math, zlib,
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
