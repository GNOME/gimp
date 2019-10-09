module_name = 'libgimp3'


gnome.gtkdoc(module_name,
  main_sgml: module_name + '-docs.sgml',
  gobject_typesfile: files(module_name + '.types'),
  src_dir: meson.source_root() / 'libgimp',

  ignore_headers: [
    'gimp-debug.h',
    'gimp-private.h',
    'gimp-shm.h',
    'gimppdb-private.h',
    'gimppdbprocedure.h',
    'gimpgpparams.h',
    'gimppixbuf.h',
    'gimpplugin-private.h',
    'gimpprocedure-private.h',
    'gimpprocedureconfig-private.h',
    'gimptilebackendplugin.h',
    'gimpunit_pdb.h',
    'gimpunitcache.h',
    'libgimp-intl.h',
    'stdplugins-intl.h',
  ],

  dependencies: [
    fontconfig, freetype2, gtk3, harfbuzz, pangocairo,
    declare_dependency(
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
  ],

  scan_args: scan_args_common,
  mkdb_args: mkdb_args_common,

  fixxref_args:  [
    '--extra-dir='+ devel_docs_build_root / 'libgimpbase'    / 'html',
    '--extra-dir='+ devel_docs_build_root / 'libgimpcolor'   / 'html',
    '--extra-dir='+ devel_docs_build_root / 'libgimpconfig'  / 'html',
    '--extra-dir='+ devel_docs_build_root / 'libgimpmath'    / 'html',
    '--extra-dir='+ devel_docs_build_root / 'libgimpwidgets' / 'html',
  ],
)
