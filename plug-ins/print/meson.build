if have_print


plugin_name = 'print'

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
    'print-draw-page.c',
    'print-page-layout.c',
    'print-page-setup.c',
    'print-preview.c',
    'print-settings.c',
    'print-utils.c',
    'print.c',
    rc_file,
  ],
  include_directories: [ rootInclude, ],
  dependencies: [
    gtk3, gegl,
    # x11, xmu, xext, xfixes,
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

endif
