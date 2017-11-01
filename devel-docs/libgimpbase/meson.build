module_name = 'libgimpbase3'


gnome.gtkdoc(module_name,
  main_sgml: module_name + '-docs.sgml',

  gobject_typesfile: files(module_name + '.types'),

  src_dir: meson.source_root() / 'libgimpbase',

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
    declare_dependency(link_with: [
      libgimpbase,
    ]),
  ],
  scanobjs_args: [
  ],
  scan_args: scan_args_common,
  mkdb_args: mkdb_args_common,
  fixxref_args:  [
  ],
  html_assets: [
  ],
  html_args: [
  ],
)
