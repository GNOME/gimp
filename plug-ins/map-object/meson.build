plugin_name = 'map-object'

plugin_sourcecode = [
  'arcball.c',
  'map-object-apply.c',
  'map-object-icons.c',
  'map-object-image.c',
  'map-object-main.c',
  'map-object-preview.c',
  'map-object-shade.c',
  'map-object-ui.c',
]
plugin_sources = plugin_sourcecode + lighting_icon_sources

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
                        win_subsystem: 'windows',
                        install: true,
                        install_dir: gimpplugindir / 'plug-ins' / plugin_name)
plugin_executables += [plugin_exe.full_path()]
