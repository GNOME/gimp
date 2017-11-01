module_name = 'libgimpbase'

version_data = configuration_data()
version_data.set('GIMP_VERSION', gimp_version)
configure_file(
  input : 'version.in',
  output: 'version',
  configuration: version_data,
)

gnome.gtkdoc(module_name,
  main_sgml: module_name + '-docs.sgml',
  gobject_typesfile: join_paths(meson.current_source_dir(), module_name + '.types'),
  src_dir: join_paths(meson.source_root(), module_name),

  content_files: [
  ],
  ignore_headers: [
    'gimpbase-private.h',
    'gimpbase.h',
    'gimpcompatenums.h',
    'gimpprotocol.h',
    'gimpreloc.h',
    'gimpwin32-io.h',
    'gimpwire.h',
  ],

  dependencies: [
    fontconfig, freetype2, gtk2, harfbuzz, pangocairo,
  ],
  scanobjs_args: [
  ],
  scan_args: [
    '--deprecated-guards=GIMP_DISABLE_DEPRECATED',
  ],
  mkdb_args: [
    '--name-space=gimp',
  ],
  fixxref_args:  [
  ],
  html_assets: [
  ],
  html_args: [
  ],
)
