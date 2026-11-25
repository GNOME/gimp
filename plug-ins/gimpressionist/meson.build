subdir('Brushes')
subdir('Paper')
subdir('Presets')

plugin_name = 'gimpressionist'

plugin_sourcecode = [
  'brush.c',
  'color.c',
  'general.c',
  'gimp.c',
  'gimpressionist.c',
  'globals.c',
  'orientation.c',
  'orientmap.c',
  'paper.c',
  'placement.c',
  'plasma.c',
  'ppmtool.c',
  'presets.c',
  'preview.c',
  'repaint.c',
  'size.c',
  'sizemap.c',
  'utils.c',
]
plugin_sources = plugin_sourcecode

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

plugin_link_args = []
plugin_link_depends = []
if platform_osx
  plugin_link_args += [ '-Wl,-sectcreate,__TEXT,__info_plist,' + gimp_plugins_plist ]
  plugin_link_depends += gimp_plugins_plist_file
endif

plugin_exe = executable(plugin_name,
                        plugin_sources,
                        dependencies: [
                          libgimpui_dep,
                          math,
                        ],
                        c_args: [
                          '-DDEFAULTPATH="@0@:@1@"'.format(
                            '~/'   / gimpdir      / 'gimpressionist',
                            prefix / gimpdatadir  / 'gimpressionist'
                          ),
                        ],
                        link_args: plugin_link_args,
                        link_depends: plugin_link_depends,
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
