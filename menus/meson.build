menus_dir = prefix / gimpdatadir / 'menus'

ui_menus_files = files(
  'app-menu.ui',
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
  'paths-menu.ui',
  'patterns-menu.ui',
  'quick-mask-menu.ui',
  'sample-points-menu.ui',
  'selection-menu.ui',
  'templates-menu.ui',
  'text-editor-toolbar.ui',
  'text-tool-menu.ui',
  'tool-options-menu.ui',
  'tool-path-menu.ui',
  'tool-preset-editor-menu.ui',
  'tool-presets-menu.ui',
  'undo-menu.ui',
)

install_data(ui_menus_files,
  install_dir: menus_dir,
)

unstable_menus_args = stable ? [] : [ '--stringparam', 'unstable-menus', 'yes' ]

menus_ui_built_files = []
foreach menu_filegen : [ 'dockable-menu.ui', 'image-menu.ui', ]
  # This does look a bit overly complicated, but I encountered 2 issues:
  # 1. The simpler solution was to do first the custom_target() then a single
  #    configure_file() per file. It didn't work out because of meson complex
  #    dependency logic (see https://github.com/mesonbuild/meson/issues/8123).
  # 2. So I inverted, but now xsltproc was the one acting up by adding the
  #    'xml:base' attribute when the included and including XML files are in
  #    different folders. This is why I added a second configure_file() to have
  #    both XML files in the same folder.
  conf = configuration_data()
  if menu_filegen == 'dockable-menu.ui'
    group = 'dockable'
  else
    group = 'app'
  endif
  conf.set('GROUP', group)
  included_file = configure_file(
    input: 'dialogs-menuitems.ui.in',
    output: group + '-dialogs-menuitems.ui',
    configuration: conf,
  )

  pre_built_file = configure_file(
    input: menu_filegen + '.in.in',
    output: menu_filegen + '.in',
    configuration: conf,
  )

  menus_ui_built_files += custom_target(menu_filegen,
    input : [ pre_built_file, 'menus.xsl', included_file],
    output: [ menu_filegen ],
    command: [
      xsltproc,
      '--xinclude',
      unstable_menus_args,
      '--path',
      meson.current_build_dir(),
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
