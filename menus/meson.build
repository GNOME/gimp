menus_dir = prefix / gimpdatadir / 'menus'

ui_menus_files = files(
  'brush-editor-menu.ui',
  'brushes-menu.ui',
  'buffers-menu.ui',
  'channels-menu.ui',
  'colormap-menu.ui',
  'cursor-info-menu.ui',
  'dashboard-menu.ui',
  'documents-menu.ui',
  'dynamics-editor-menu.ui',
  'dynamics-menu.ui',
  'error-console-menu.ui',
  'fonts-menu.ui',
  'gradient-editor-menu.ui',
  'gradients-menu.ui',
  'images-menu.ui',
  'layers-menu.ui',
  'mypaint-brushes-menu.ui',
  'palette-editor-menu.ui',
  'palettes-menu.ui',
  'patterns-menu.ui',
  'quick-mask-menu.ui',
  'sample-points-menu.ui',
  'selection-menu.ui',
  'templates-menu.ui',
  'text-editor-toolbar.ui',
  'text-tool-menu.ui',
  'tool-options-menu.ui',
  'tool-preset-editor-menu.ui',
  'tool-presets-menu.ui',
  'undo-menu.ui',
  'vector-toolpath-menu.ui',
  'vectors-menu.ui',
)

install_data(ui_menus_files,
  install_dir: menus_dir,
)

unstable_menus_args = stable ? [] : [ '--stringparam', 'unstable-menus', 'yes' ]

menus_ui_built_files = []
foreach menu_filegen : [ 'dockable-menu.ui', 'image-menu.ui', ]
  menus_ui_built_files += custom_target(menu_filegen,
    input : [ menu_filegen +'.in', 'menus.xsl', 'dialogs-menuitems.ui'],
    output: [ menu_filegen ],
    command: [
      xsltproc,
      '--xinclude',
      unstable_menus_args,
      '--output', '@OUTPUT@',
      '@INPUT1@',
      '@INPUT0@',
    ],
    install: true,
    install_dir: menus_dir,
  )
endforeach

if xmllint.found()
  # XXX: no DTD validation as GtkBuilder UI format does not have a DTD (as far as
  # we could find).
  custom_target('validate_ui_menus',
    command: [
      xmllint,
      '--output', '@OUTPUT@',
      '--path', meson.current_source_dir(),
      ui_menus_files, menus_ui_built_files
    ],
    # The output file is only useful as a flag file, so that the command
    # knows if it has been run already.
    output: [ 'validate_ui_menus-output.xml' ],
    build_by_default: true,
    install: false
  )
endif
