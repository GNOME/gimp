module_name = 'libgimpmodule3'


gnome.gtkdoc(module_name,
  main_sgml: module_name + '-docs.sgml',
  gobject_typesfile: files(module_name + '.types'),
  src_dir: meson.source_root() / 'libgimpmodule',

  dependencies: [
    fontconfig, freetype2, gtk3, harfbuzz, pangocairo,
    declare_dependency(
      include_directories: rootInclude,
      link_with: [
        libgimpmodule,
      ],
    )
  ],

  scan_args: scan_args_common + [
    '--deprecated-guards="GIMP_DISABLE_DEPRECATED"'
  ],
  mkdb_args: mkdb_args_common,
)
