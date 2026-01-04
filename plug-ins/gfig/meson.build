subdir('gfig-examples')
subdir('images')

plugin_name = 'gfig'

plugin_sourcecode = [
  'gfig-arc.c',
  'gfig-bezier.c',
  'gfig-circle.c',
  'gfig-dialog.c',
  'gfig-dobject.c',
  'gfig-ellipse.c',
  'gfig-grid.c',
  'gfig-icons.c',
  'gfig-line.c',
  'gfig-poly.c',
  'gfig-preview.c',
  'gfig-rectangle.c',
  'gfig-spiral.c',
  'gfig-star.c',
  'gfig-style.c',
  'gfig.c',
]
plugin_sources = plugin_sourcecode + gfig_icon_sources

plugin_sources += gnome.compile_resources(
  'gfig-menus',
  'gfig-menus.gresource.xml',
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
                          math,
                        ],
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
plugin_executables += [plugin_exe.full_path()]
