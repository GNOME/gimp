module_name = 'libgimpcolor3'


gnome.gtkdoc(module_name,
  main_sgml: module_name + '-docs.sgml',
  gobject_typesfile: files(module_name + '.types'),
  src_dir: meson.source_root() / 'libgimpcolor',

  ignore_headers: [
  ],

  dependencies: [
    fontconfig, freetype2, gtk3, harfbuzz, pangocairo,
    declare_dependency(link_with: [
      libgimpbase,
      libgimpcolor,
    ]),
  ],
  scan_args: scan_args_common,
  mkdb_args: mkdb_args_common,
)
