subdir('images')

plugin_name = 'imagemap'

plugin_sources = [
  'imap_about.c',
  'imap_browse.c',
  'imap_cern_lex.c',
  'imap_cern_parse.c',
  'imap_circle.c',
  'imap_cmd_clear.c',
  'imap_cmd_copy_object.c',
  'imap_cmd_copy.c',
  'imap_cmd_create.c',
  'imap_cmd_cut_object.c',
  'imap_cmd_cut.c',
  'imap_cmd_delete_point.c',
  'imap_cmd_delete.c',
  'imap_cmd_edit_object.c',
  'imap_cmd_gimp_guides.c',
  'imap_cmd_guides.c',
  'imap_cmd_insert_point.c',
  'imap_cmd_move_down.c',
  'imap_cmd_move_sash.c',
  'imap_cmd_move_selected.c',
  'imap_cmd_move_to_front.c',
  'imap_cmd_move_up.c',
  'imap_cmd_move.c',
  'imap_cmd_object_down.c',
  'imap_cmd_object_move.c',
  'imap_cmd_object_up.c',
  'imap_cmd_paste.c',
  'imap_cmd_select_all.c',
  'imap_cmd_select_next.c',
  'imap_cmd_select_prev.c',
  'imap_cmd_select_region.c',
  'imap_cmd_select.c',
  'imap_cmd_send_to_back.c',
  'imap_cmd_unselect_all.c',
  'imap_cmd_unselect.c',
  'imap_command.c',
  'imap_csim_lex.c',
  'imap_csim_parse.c',
  'imap_default_dialog.c',
  'imap_edit_area_info.c',
  'imap_file.c',
  'imap_grid.c',
  'imap_icons.c',
  'imap_main.c',
  'imap_menu_funcs.c',
  'imap_menu.c',
  'imap_misc.c',
  'imap_mru.c',
  'imap_ncsa_lex.c',
  'imap_ncsa_parse.c',
  'imap_object_popup.c',
  'imap_object.c',
  'imap_polygon.c',
  'imap_preferences.c',
  'imap_preview.c',
  'imap_rectangle.c',
  'imap_selection.c',
  'imap_settings.c',
  'imap_source.c',
  'imap_statusbar.c',
  'imap_string.c',
  'imap_taglist.c',
  'imap_ui_grid.c',
  imagemap_icon_sources,
]

if platform_windows
  plugin_sources += windows.compile_resources(
    gimp_plugins_rc,
    args: [
      '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plugin_name+'.exe'),
      '--define', 'INTERNALNAME_STR="@0@"'    .format(plugin_name),
      '--define', 'TOP_SRCDIR="@0@"'          .format(meson.source_root()),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
endif

executable(plugin_name,
  plugin_sources,
  dependencies: [
    libgimpui_dep,
    math,
  ],
  install: true,
  install_dir: gimpplugindir / 'plug-ins' / plugin_name,
)



flex  = find_program('flex',  required: false)
bison = find_program('bison', required: false)

if flex.found() and bison.found()

  syntaxes = [
    'cern',
    'csim',
    'ncsa',
  ]

  foreach syntax : syntaxes

    syntax_prefix =  syntax + '_'
    imapname = 'imap_' + syntax

    custom_target(
              imapname + '_lex.c',
      input : imapname + '.l'    ,
      output: imapname + '_lex.c',
      command: [
        flex,
        '--case-insensitive',
        '--prefix', syntax_prefix,
        '--outfile', '@OUTPUT@',
        '@INPUT@',
      ],
    )

    custom_target(
              imapname + '_parse.c',
      input : imapname + '.y'      ,
      output: imapname + '_parse.c',
      command: [
        bison, '--yacc',
        '-d',
        '--name-prefix', syntax_prefix,
        '@INPUT@'
      ],
    )

    custom_target(
              imapname + '_parse.h',
      input : imapname + '.y'      ,
      output: imapname + '_parse.h',
      command: [
        bison, '--yacc',
        '-d',
        '--name-prefix', syntax_prefix,
        '--output', '@OUTPUT@',
        '@INPUT@'
      ],
    )

  endforeach
endif
