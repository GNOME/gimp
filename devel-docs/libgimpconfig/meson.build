module_name = 'libgimpconfig3'

version_data = configuration_data()
version_data.set('GIMP_VERSION', gimp_version)
configure_file(
  input : 'version.in',
  output: 'version',
  configuration: version_data,
)

gnome.gtkdoc(module_name,
  main_sgml: module_name + '-docs.sgml',
  gobject_typesfile: files(module_name + '.types'),
  src_dir: meson.current_source_dir(),

  content_files: [
  ],
  ignore_headers: [
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
    '--extra-dir='+ meson.current_build_dir() /'..'/ 'libgimpbase'  / 'html',
    '--extra-dir='+ meson.current_build_dir() /'..'/ 'libgimpcolor' / 'html',
    '--extra-dir='+ meson.current_build_dir() /'..'/ 'libgimpmath'  / 'html',
  ],
  html_assets: [
  ],
  html_args: [
  ],
)
