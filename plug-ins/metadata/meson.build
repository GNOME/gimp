
plugin_name = 'metadata-editor'

plugin_rc = configure_file(
  input : gimp_plugins_rc,
  output: plugin_name + '.rc',
  copy: true,
)
rc_file = platform_win32 ? windows.compile_resources(
  plugin_rc,
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
    'metadata-editor.c',
    'metadata-impexp.c',
    'metadata-xml.c',
    rc_file,
  ],
  include_directories: [ rootInclude, ],
  dependencies: [
    gtk3, gegl, gexiv2,
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
  c_args: '-DGIMP_DEPRECATED_REPLACE_NEW_API',
  install: true,
  install_dir: gimpplugindir / 'plug-ins' / plugin_name,
)

plugin_name = 'metadata-viewer'

plugin_rc = configure_file(
  input : gimp_plugins_rc,
  output: plugin_name + '.rc',
  copy: true,
)
rc_file = platform_win32 ? windows.compile_resources(
  plugin_rc,
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
    'metadata-viewer.c',
  ],
  include_directories: [ rootInclude, ],
  dependencies: [
    gtk3, gegl, gexiv2,
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
  install_dir: gimpplugindir / 'plug-ins' / plugin_name,
)
