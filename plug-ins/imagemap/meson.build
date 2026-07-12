subdir('images')

plugin_name = 'imagemap'

plugin_sourcecode = [
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
]
plugin_sources = plugin_sourcecode + imagemap_icon_sources

if platform_windows
  plugin_sources += windows.compile_resources(
    gimp_plugins_rc,
    args: [
      rc_define_flag, rc_define_fmt.format('ORIGINALFILENAME_STR', plugin_name+'.exe'),
      rc_define_flag, rc_define_fmt.format('INTERNALNAME_STR', plugin_name),
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

#TODO: on meson 1.10+, use 'codegen' module
if not platform_windows
  flex  = find_program('flex',  required: false)
  bison = find_program('bison', required: false)
else
  #ensure we use native binaries from winflexbison, not the CYGWIN ones from MSYS2
  flex  = find_program('win_flex',  required: false)
  bison = find_program('win_bison', required: false)
endif

menu_config = configuration_data()
toolbar_compiler_args = []
if flex.found() and bison.found()
  #Only show Open menu if imagemap was built with flex and bison (see issue #14145)
  toolbar_compiler_args += '-DHAVE_FLEX_BISON'
  multiline_xml = '''<item>
                      <attribute name="label" translatable="yes">Open</attribute>
                      <attribute name="action">app.open</attribute>
                     </item>'''
  menu_config.set('OPTIONAL_OPEN_MENU', multiline_xml)

  if cc.get_argument_syntax() == 'msvc'
    toolbar_compiler_args += ['-DYY_NO_UNISTD_H', '-Disatty(x)=0', '-Wno-deprecated-declarations']
  endif

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

menu_ui = configure_file(
  input: 'imap-menu.ui.in',
  output: 'imap-menu.ui',
  configuration: menu_config
)

plugin_sources += gnome.compile_resources(
  'imagemap-menus',
  'imagemap-menus.gresource.xml',
  dependencies: menu_ui,
  source_dir: meson.current_build_dir()
)

plugin_exe = executable(plugin_name,
                        plugin_sources,
                        dependencies: [
                          libgimpui_dep,
                          math,
                        ],
                        c_args: toolbar_compiler_args,
                        win_subsystem: 'windows',
                        install: true,
                        install_dir: gimpplugindir / 'plug-ins' / plugin_name)

# Ugly trick to add executables to plugin_custom_targets list (see root meson.build)
plugin_custom_targets += custom_target('dummy-' + plugin_name,
              input: plugin_exe,
              output: 'dummy-' + plugin_name + '.dummy',
              command: [python, '-c',
                        'open("@OUTPUT@", "w").write("Nice. I was built before calling in-build-gimp.py\\n")'],
              build_by_default: true)
