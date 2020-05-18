menus_dir = prefix / gimpdatadir / 'menus'

menus_files = [
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
]

install_data(menus_files,
  install_dir: menus_dir,
)

unstable_menus_args = stable ? [] : [ '--stringparam', 'unstable-menus', 'yes' ]

foreach menu_filegen : [ 'dockable-menu.xml', 'image-menu.xml', ]
  menus_files += custom_target(menu_filegen,
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
  run_target('validate_menus',
    command: [
      xmllint,
      '--noout',
      '--path', meson.current_source_dir(),
      '@INPUT@', menus_files,
    ],
  )
endif
