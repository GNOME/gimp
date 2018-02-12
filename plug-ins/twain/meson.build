if platform_win32

plugin_name = 'twain'

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

# Win32 specific
plugin_twain_sources = [
  'tw_func.c',
  'tw_util.c',
  'tw_win.c',
  'twain.c',
  rc_file,
]

twain = executable(plugin_name,
  plugin_twain_sources,
  include_directories: [ rootInclude, ],
  dependencies: [
    glib, gtk3, gegl,
  ],
  link_with: [
    libgimp,
    libgimpcolor,
    libgimpbase,
  ],
  install: true,
  install_dir: join_paths(gimpplugindir, 'plug-ins'),
)

endif
