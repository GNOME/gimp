module_name = 'libgimpthumb3'


gnome.gtkdoc(module_name,
  main_sgml: module_name + '-docs.sgml',
  gobject_typesfile: files(module_name + '.types'),
  src_dir: meson.current_source_dir(),

  content_files: [
  ],
  ignore_headers: [
    'libgimpthumb.h',
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
    '--extra-dir='+ meson.current_build_dir() /'..'/ 'libgimpmath' / 'html',
  ],
  html_assets: [
  ],
  html_args: [
  ],
)
