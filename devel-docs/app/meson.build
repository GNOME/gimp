module_name = 'app'


gnome.gtkdoc(module_name,
  main_sgml: module_name + '-docs.sgml',
  gobject_typesfile: files(module_name + '.types'),
  src_dir: meson.source_root() / 'app',

  content_files: [],
  ignore_headers: [
    'gimp-intl.h',
    'gimpdbusservice-glue.h',
    'gimphelp-ids.h',
    'gimpimage-convert-data.h',
    'gimpimage-convert-fsdither.h',
    'gimprc-blurbs.h',
  ],

  dependencies: [
    fontconfig, freetype2, gtk3, harfbuzz, pangocairo,
    declare_dependency(link_with: [
      libappactions,
      libappconfig,
      libappcore,
      libappdialogs,
      libappdisplay,
      libappfile,
      libappgegl,
      libappgui,
      libappinternalprocs,
      libappmenus,
      libappoperations,
      libapppaint,
      libapppdb,
      libappplugin,
      libapppropgui,
      libapptext,
      libapptools,
      libappvectors,
      libappwidgets,
      libappxcf,

      libgimpwidgets,
      libgimpmodule,
      libgimpcolor,
      libgimpthumb,
      libgimpmath,
      libgimpconfig,
      libgimpbase,
    ])
  ],

  scanobjs_args: [
  ],
  scan_args: [
  ],
  mkdb_args: [
  ],
  c_args: [
    '-UGTK_DISABLE_SINGLE_INCLUDES',
  ],
  fixxref_args:  [
    '--extra-dir=' + devel_docs_build_root / 'libgimpbase'    / 'html',
    '--extra-dir=' + devel_docs_build_root / 'libgimpcolor'   / 'html',
    '--extra-dir=' + devel_docs_build_root / 'libgimpconfig'  / 'html',
    '--extra-dir=' + devel_docs_build_root / 'libgimpmath'    / 'html',
    '--extra-dir=' + devel_docs_build_root / 'libgimpwidgets' / 'html',
  ],
  html_assets: [
  ],
  html_args: [
  ],

  install: true,
)
