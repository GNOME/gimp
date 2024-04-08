subdir('images')

plugin_name = 'imagemap'

plugin_sources = [
  'imap_about.c',
  'imap_browse.c',
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

plugin_sources += gnome.compile_resources(
  'imagemap-menus',
  'imagemap-menus.gresource.xml',
)

if platform_windows
  plugin_sources += windows.compile_resources(
    gimp_plugins_rc,
    args: [
      '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plugin_name+'.exe'),
      '--define', 'INTERNALNAME_STR="@0@"'    .format(plugin_name),
      '--define', 'TOP_SRCDIR="@0@"'          .format(meson.project_source_root()),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
endif

syntaxes = [
  'cern',
  'csim',
  'ncsa',
]

flex  = find_program('flex',  required: false)
bison = find_program('bison', required: false)

if flex.found() and bison.found()
  lex_gen = generator(
                flex,
     arguments: [
        '--case-insensitive',
        '@EXTRA_ARGS@',
        '--outfile', '@OUTPUT@',
        '@INPUT@',
      ],
      output  : '@BASENAME@_lex.c'
  )

  parse_gen = generator(
                bison,
     arguments: [
        '--yacc',
        '-d',
        '@EXTRA_ARGS@',
        '--output', '@OUTPUT0@',
        '--defines=@OUTPUT1@',
        '@INPUT@'
      ],
      output  : [
        '@BASENAME@_parse.c',
        '@BASENAME@_parse.h',
      ]
  )

  foreach syntax : syntaxes
    syntax_prefix =  syntax + '_'
    imapname = 'imap_' + syntax

    plugin_sources += lex_gen.process(
                  imapname + '.l',
      extra_args: ['--prefix', syntax_prefix]
    )
    plugin_sources += parse_gen.process(
                  imapname + '.y',
      extra_args: ['--name-prefix', syntax_prefix]
    )
  endforeach
else
  foreach syntax : syntaxes
    syntax_prefix =  syntax + '_'
    imapname = 'imap_' + syntax

    plugin_sources += [
      imapname + '_lex.c',
      imapname + '_parse.c',
      imapname + '_parse.h'
    ]
  endforeach
endif

plugin_exe = executable(plugin_name,
                        plugin_sources,
                        dependencies: [
                          libgimpui_dep,
                          math,
                        ],
                        install: true,
                        install_dir: gimpplugindir / 'plug-ins' / plugin_name)
plugin_executables += [plugin_exe.full_path()]
