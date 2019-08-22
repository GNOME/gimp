module_name = 'libgimpbase3'


gnome.gtkdoc(module_name,
  main_sgml: module_name + '-docs.sgml',
  gobject_typesfile: files(module_name + '.types'),
  src_dir: meson.current_source_dir(),

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
    fontconfig, freetype2, gtk3, harfbuzz, pangocairo,
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
