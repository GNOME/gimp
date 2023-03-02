menus_dir = prefix / gimpdatadir / 'menus'

menus_files = files(
  'brush-editor-menu.xml',
  'brushes-menu.xml',
  'buffers-menu.xml',
  'channels-menu.xml',
  'colormap-menu.xml',
  'cursor-info-menu.xml',
  'dashboard-menu.xml',
  'documents-menu.xml',
  'dynamics-editor-menu.xml',
  'dynamics-menu.xml',
  'error-console-menu.xml',
  'fonts-menu.xml',
  'gradient-editor-menu.xml',
  'gradients-menu.xml',
  'images-menu.xml',
  'layers-menu.xml',
  'mypaint-brushes-menu.xml',
  'palette-editor-menu.xml',
  'palettes-menu.xml',
  'patterns-menu.xml',
  'quick-mask-menu.xml',
  'sample-points-menu.xml',
  'selection-menu.xml',
  'templates-menu.xml',
  'text-editor-toolbar.xml',
  'text-tool-menu.xml',
  'tool-options-menu.xml',
  'tool-preset-editor-menu.xml',
  'tool-presets-menu.xml',
  'undo-menu.xml',
  'vector-toolpath-menu.xml',
  'vectors-menu.xml',
)

install_data(menus_files,
  install_dir: menus_dir,
)

ui_menus_files = files(
  'brushes-menu.ui',
  'buffers-menu.ui',
  'channels-menu.ui',
  'colormap-menu.ui',
  'dashboard-menu.ui',
  'documents-menu.ui',
  'dynamics-menu.ui',
  'error-console-menu.ui',
  'fonts-menu.ui',
  'gradient-editor-menu.ui',
  'gradients-menu.ui',
  'images-menu.ui',
  'layers-menu.ui',
  'mypaint-brushes-menu.ui',
  'palettes-menu.ui',
  'patterns-menu.ui',
  'quick-mask-menu.ui',
  'templates-menu.ui',
  'text-editor-toolbar.ui',
  'undo-menu.ui',
  'vectors-menu.ui',
)

install_data(ui_menus_files,
  install_dir: menus_dir,
)

unstable_menus_args = stable ? [] : [ '--stringparam', 'unstable-menus', 'yes' ]

menus_built_files = []
foreach menu_filegen : [ 'dockable-menu.xml', 'image-menu.xml', ]
  menus_built_files += custom_target(menu_filegen,
    input : [ menu_filegen +'.in', 'menus.xsl', ],
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

menus_ui_built_files = []
foreach menu_filegen : [ 'dockable-menu.ui', 'image-menu.ui', ]
  menus_ui_built_files += custom_target(menu_filegen,
    input : [ menu_filegen +'.in', 'menus.xsl', ],
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
  custom_target('validate_menus',
    command: [
      xmllint,
      '--output', '@OUTPUT@',
      '--valid',
      '--path', meson.current_source_dir(),
      menus_files, menus_built_files
    ],
    # The output file is only useful as a flag file, so that the command
    # knows if it has been run already.
    output: [ 'validate_menus-output.xml' ],
    build_by_default: true,
    install: false
  )

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
