subdir('gfig-examples')
subdir('images')

plugin_name = 'gfig'

plugin_sources = [
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
  gfig_icon_sources,
]

plugin_sources += gnome.compile_resources(
  'gfig-menus',
  'gfig-menus.gresource.xml',
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

plugin_exe = executable(plugin_name,
                        plugin_sources,
                        dependencies: [
                          libgimpui_dep,
                          math,
                        ],
                        install: true,
                        install_dir: gimpplugindir / 'plug-ins' / plugin_name)
plugin_executables += [plugin_exe.full_path()]
