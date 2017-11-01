module_name = 'libgimpmath3'


gnome.gtkdoc(module_name,
  main_sgml: module_name + '-docs.sgml',
  gobject_typesfile: files(module_name + '.types'),
  src_dir: meson.source_root() / 'libgimpmath',

  dependencies: [
    fontconfig, freetype2, gtk3, harfbuzz, pangocairo,
  ],
  scan_args: scan_args_common,
  mkdb_args: mkdb_args_common,
)
