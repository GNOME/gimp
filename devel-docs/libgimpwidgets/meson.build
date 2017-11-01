module_name = 'libgimpwidgets'

version_data = configuration_data()
version_data.set('GIMP_VERSION', gimp_version)
configure_file(
  input : 'version.in',
  output: 'version',
  configuration: version_data,
)

gnome.gtkdoc(module_name,
  main_sgml: module_name + '-docs.sgml',
  gobject_typesfile: join_paths(meson.current_source_dir(), module_name + '.types'),
  src_dir: join_paths(meson.source_root(), module_name),

  content_files: [
    'visual-index.html'
  ],
  ignore_headers: [
    'gimpcolorprofilestore-private.h',
    'gimpeevl.h',
    'gimpwidgets-private.h',
  ],

  dependencies: [
    fontconfig, freetype2, gtk2, harfbuzz, pangocairo,
  ],
  scanobjs_args: [
    '--type-init-func="do { void babl_init (void); babl_init (); } while (0)"',
  ],
  scan_args: [
    '--deprecated-guards=GIMP_DISABLE_DEPRECATED',
  ],
  mkdb_args: [
    '--name-space=gimp',
  ],
  fixxref_args:  [
    '--extra-dir='+ join_paths(meson.current_build_dir(), '..', 'libgimp',      'html'),
    '--extra-dir='+ join_paths(meson.current_build_dir(), '..', 'libgimpbase',  'html'),
    '--extra-dir='+ join_paths(meson.current_build_dir(), '..', 'libgimpcolor', 'html'),
    '--extra-dir='+ join_paths(meson.current_build_dir(), '..', 'libgimpconfig','html'),
    '--extra-dir='+ join_paths(meson.current_build_dir(), '..', 'libgimpmath',  'html'),
  ],
  html_assets: [
    # '$(top_srcdir)/icons/Color/12/gimp-*.png',
    # '$(top_srcdir)/icons/Color/16/gimp-*.png',
    # '$(top_srcdir)/icons/Color/18/gimp-*.png',
    # '$(top_srcdir)/icons/Color/20/gimp-*.png',
    # '$(top_srcdir)/icons/Color/22/gimp-*.png',
    # '$(top_srcdir)/icons/Color/24/gimp-*.png',
    # '$(top_srcdir)/icons/Color/32/gimp-*.png',
    # '$(top_srcdir)/icons/Color/48/gimp-*.png',
    # '$(top_srcdir)/icons/Color/64/gimp-*.png',
    # '$(top_srcdir)/icons/Color/128/gimp-*.png',
    # '$(top_srcdir)/icons/Color/192/gimp-*.png',
    # '$(top_srcdir)/icons/Color/256/gimp-*.png',
    # '$(srcdir)/images/gimp-widget-*.png',
  ],
  html_args: [
  ],
)
