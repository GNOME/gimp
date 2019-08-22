module_name = 'app'

app_libs = shared_library('app',
  include_directories: [ rootInclude, rootAppInclude, configInclude, ],

  link_whole: [

    libappconfig,
    libappcore,
    libappfile,
    libappfiledata,
    libappgegl,
    libappinternalprocs,
    libapplayermodes,
    libapplayermodeslegacy,
    libappoperations,
    libapppaint,
    libapppdb,
    libappplugin,
    libapptext,
    libappvectors,
    libappxcf,

    libappactions,
    libappdialogs,
    libappdisplay,
    libappgui,
    libappmenus,
    libapppropgui,
    libapptools,
    libappwidgets,
  ],

  link_with: [
    libgimpbase,
    libgimpcolor,
    libgimpconfig,
    libgimpmath,
    libgimpmodule,
    libgimpthumb,
    libgimpwidgets,
  ],
  dependencies: [
    gegl, gdk_pixbuf, gtk3,
  ],
)


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
    gegl,
    declare_dependency(
      include_directories: rootInclude,
      link_with: [
      app_libs,

      libgimpbase,
      libgimpcolor,
      libgimpconfig,
      libgimpmath,
      libgimpmodule,
      libgimpthumb,
      libgimpwidgets,
    ])
  ],

  scan_args: scan_args_common,
  mkdb_args: mkdb_args_common,
  c_args: [
    '-UGTK_DISABLE_SINGLE_INCLUDES',
    '-fdiagnostics-color=always',
  ],

  fixxref_args:  [
    '--extra-dir=' + devel_docs_build_root / 'libgimpbase'    / 'html',
    '--extra-dir=' + devel_docs_build_root / 'libgimpcolor'   / 'html',
    '--extra-dir=' + devel_docs_build_root / 'libgimpconfig'  / 'html',
    '--extra-dir=' + devel_docs_build_root / 'libgimpmath'    / 'html',
    '--extra-dir=' + devel_docs_build_root / 'libgimpwidgets' / 'html',
  ],

  install: true,
)
