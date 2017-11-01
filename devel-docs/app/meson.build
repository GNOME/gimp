module_name = 'app'

version_data = configuration_data()
version_data.set('GIMP_VERSION', gimp_version)
configure_file(
  input : 'version.in',
  output: 'version',
  configuration: version_data,
)

gnome.gtkdoc(module_name,
  content_files: [],
  dependencies: [
    fontconfig, freetype2, gtk2, harfbuzz, pangocairo,
  ],
  fixxref_args:  [
    '--extra-dir=' + join_paths(meson.current_build_dir(), '..', 'libgimpbase',    'html'),
    '--extra-dir=' + join_paths(meson.current_build_dir(), '..', 'libgimpcolor',   'html'),
    '--extra-dir=' + join_paths(meson.current_build_dir(), '..', 'libgimpconfig',  'html'),
    '--extra-dir=' + join_paths(meson.current_build_dir(), '..', 'libgimpmath',    'html'),
    '--extra-dir=' + join_paths(meson.current_build_dir(), '..', 'libgimpwidgets', 'html'),
  ],
  gobject_typesfile: join_paths(meson.current_source_dir(), module_name + '.types'),
  scanobjs_args: [
    '--cflags=-I'+ meson.source_root(),
  ],
  ignore_headers: [
    'gimp-intl.h',
    'gimpdbusservice-glue.h',
    'gimphelp-ids.h',
    'gimpimage-convert-data.h',
    'gimpimage-convert-fsdither.h',
    'gimprc-blurbs.h',
  ],
  html_assets: [],
  html_args: [],
  main_sgml: module_name + '-docs.sgml',
  src_dir: join_paths(meson.source_root(), module_name),

  install: true,
  # install_dir: ,
)
