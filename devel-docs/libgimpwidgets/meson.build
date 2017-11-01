module_name = 'libgimpwidgets3'


gnome.gtkdoc(module_name,
  main_sgml: module_name + '-docs.sgml',
  gobject_typesfile: files(module_name + '.types'),
  src_dir: meson.source_root() / 'libgimpwidgets',

  content_files: [
    'visual-index.xml'
  ],
  ignore_headers: [
    'gimp-color-picker-cursors.h',
    'gimp-icon-pixbufs.h',
    'gimpcolorprofilestore-private.h',
    'gimpeevl.h',
    'gimpwidgets-private.h',
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
      libgimpwidgets,
    ]),
  ],
  scanobjs_args: [
    '--type-init-func=do { void babl_init (void); babl_init (); } while (0)',
  ],
  c_args: [
    '-DGIMP_ENABLE_CONTROLLER_UNDER_CONSTRUCTION',
    '-UGIMP_DISABLE_DEPRECATED',
  ],
  scan_args: scan_args_common,
  mkdb_args: mkdb_args_common,
  fixxref_args:  [
    '--extra-dir='+ devel_docs_build_root / 'libgimp'       / 'html',
    '--extra-dir='+ devel_docs_build_root / 'libgimpbase'   / 'html',
    '--extra-dir='+ devel_docs_build_root / 'libgimpcolor'  / 'html',
    '--extra-dir='+ devel_docs_build_root / 'libgimpconfig' / 'html',
    '--extra-dir='+ devel_docs_build_root / 'libgimpmath'   / 'html',
  ],
  html_assets: [
    'images' / 'gimp-widget-browser.png',
    'images' / 'gimp-widget-busy-box.png',
    'images' / 'gimp-widget-button.png',
    'images' / 'gimp-widget-chain-button.png',
    'images' / 'gimp-widget-color-area.png',
    'images' / 'gimp-widget-color-button.png',
    'images' / 'gimp-widget-color-hex-entry.png',
    'images' / 'gimp-widget-color-notebook.png',
    'images' / 'gimp-widget-color-profile-combo-box.png',
    'images' / 'gimp-widget-color-profile-view.png',
    'images' / 'gimp-widget-color-scale.png',
    'images' / 'gimp-widget-color-scales.png',
    'images' / 'gimp-widget-color-select.png',
    'images' / 'gimp-widget-color-selection.png',
    'images' / 'gimp-widget-dialog.png',
    'images' / 'gimp-widget-enum-combo-box.png',
    'images' / 'gimp-widget-enum-label.png',
    'images' / 'gimp-widget-file-entry.png',
    'images' / 'gimp-widget-frame.png',
    'images' / 'gimp-widget-hint-box.png',
    'images' / 'gimp-widget-int-combo-box.png',
    'images' / 'gimp-widget-memsize-entry.png',
    'images' / 'gimp-widget-number-pair-entry.png',
    'images' / 'gimp-widget-offset-area.png',
    'images' / 'gimp-widget-page-selector.png',
    'images' / 'gimp-widget-path-editor.png',
    'images' / 'gimp-widget-pick-button.png',
    'images' / 'gimp-widget-preview-area.png',
    'images' / 'gimp-widget-ruler.png',
    'images' / 'gimp-widget-string-combo-box.png',
    'images' / 'gimp-widget-unit-combo-box.png',
  ],
)
