plugin_name = 'file-faxg3'

plugin_sources = [
  'faxg3.c',
  'g3.c',
]

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
                        dependencies: libgimpui_dep,
                        install: true,
                        install_dir: gimpplugindir / 'plug-ins' / plugin_name)
plugin_executables += [plugin_exe.full_path()]
