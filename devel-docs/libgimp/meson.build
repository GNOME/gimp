module_name = 'libgimp'

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
  gobject_typesfile: join_paths(meson.current_source_dir(), module_name + '.types'),
  src_dir: join_paths(meson.source_root(), module_name),

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
    fontconfig, freetype2, gtk2, harfbuzz, pangocairo,
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
  ],
  html_assets: [
  ],
  html_args: [
  ],
)
