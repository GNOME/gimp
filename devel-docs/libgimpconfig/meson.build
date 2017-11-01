module_name = 'libgimpconfig3'


gnome.gtkdoc(module_name,
  main_sgml: module_name + '-docs.sgml',
  gobject_typesfile: files(module_name + '.types'),
  src_dir: meson.source_root() / 'libgimpconfig',

  ignore_headers: [
  ],

  dependencies: [
    fontconfig, freetype2, gtk3, harfbuzz, pangocairo,
    declare_dependency(
      include_directories: rootInclude,
      link_with: [
      libgimpbase,
      libgimpcolor,
      libgimpmath,
      libgimpconfig,
    ]),
  ],
  scan_args: scan_args_common,
  mkdb_args: mkdb_args_common,
  fixxref_args:  [
    '--extra-dir='+ devel_docs_build_root / 'libgimpbase'  / 'html',
    '--extra-dir='+ devel_docs_build_root / 'libgimpcolor' / 'html',
    '--extra-dir='+ devel_docs_build_root / 'libgimpmath'  / 'html',
  ],
)
