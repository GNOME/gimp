plugin_name = 'help'

plugin_sourcecode = [
  # 'gimp-help-lookup.c',
  'gimphelp.c',
  'gimphelpdomain.c',
  'gimphelpitem.c',
  'gimphelplocale.c',
  'gimphelpprogress.c',
]
plugin_sources = plugin_sourcecode

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

help_plugin_lib = static_library('help_plugin',
  plugin_sources,
  include_directories: [ rootInclude, ],
  dependencies: [
    libgimpui_dep, gio,
  ],
  install: false,
)

plugin_exe = executable(plugin_name,
                        'help.c',
                        dependencies: [
                          libgimpui_dep,
                          gio,
                          gdk_pixbuf,
                        ],
                        link_with: [
                          help_plugin_lib,
                        ],
                        win_subsystem: 'windows',
                        install: true,
                        install_dir: gimpplugindir / 'plug-ins' / plugin_name)
plugin_executables += [plugin_exe.full_path()]
