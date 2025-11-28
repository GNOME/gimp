if get_option('webkit-unmaintained')

plugin_name = 'help-browser'

plugin_sourcecode = [
  'dialog.c',
  'help-browser.c',
  'uri.c',
]
plugin_sources = plugin_sourcecode

plugin_sources += gnome.compile_resources(
  'help-menus',
  'help-menus.gresource.xml',
)

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

plugin_exe = executable(plugin_name,
                        plugin_sources,
                        dependencies: [
                          libgimpui_dep,
                          gio,
                          webkit,
                        ],
                        link_with: [
                          help_plugin_lib,
                        ],
                        win_subsystem: 'windows',
                        install: true,
                        install_dir: gimpplugindir / 'plug-ins' / plugin_name)
plugin_executables += [plugin_exe.full_path()]

endif
