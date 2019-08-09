module_name = 'libgimp3'

version_data = configuration_data()
version_data.set('GIMP_VERSION', gimp_version)
configure_file(
  input : 'version.in',
  output: 'version',
  configuration: version_data,
)

gimp_dep = declare_dependency(
  include_directories: rootInclude,
  link_with: [
    libgimp,
    libgimpui,
    libgimpbase,
    libgimpcolor,
    libgimpconfig,
    libgimpmath,
    libgimpui,
    libgimpwidgets,
  ]
)

gnome.gtkdoc(module_name,
  main_sgml: module_name + '-docs.sgml',
  gobject_typesfile: files(module_name + '.types'),
  src_dir: meson.current_source_dir(),

  content_files: [
  ],
  ignore_headers: [
    'gimptilebackendplugin.h',
    'gimpunit_pdb.h',
    'gimpunitcache.h',
    'libgimp-intl.h',
    'stdplugins-intl.h',
  ],

  dependencies: [
    fontconfig, freetype2, gtk3, harfbuzz, pangocairo,
    gimp_dep,
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
    '--extra-dir='+ meson.current_source_dir() /'..'/ 'libgimpbase'    / 'html',
    '--extra-dir='+ meson.current_source_dir() /'..'/ 'libgimpcolor'   / 'html',
    '--extra-dir='+ meson.current_source_dir() /'..'/ 'libgimpconfig'  / 'html',
    '--extra-dir='+ meson.current_source_dir() /'..'/ 'libgimpmath'    / 'html',
    '--extra-dir='+ meson.current_source_dir() /'..'/ 'libgimpwidgets' / 'html',
  ],
  html_assets: [
  ],
  html_args: [
  ],
)
