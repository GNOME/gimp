if have_print


plugin_name = 'print'

plugin_sourcecode = [
  'print-draw-page.c',
  'print-page-layout.c',
  'print-page-setup.c',
  'print-preview.c',
  'print-settings.c',
  'print-utils.c',
  'print.c',
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

plugin_exe = executable(plugin_name,
                        plugin_sources,
                        dependencies: libgimpui_dep,
                        win_subsystem: 'windows',
                        install: true,
                        install_dir: gimpplugindir / 'plug-ins' / plugin_name)
plugin_executables += [plugin_exe.full_path()]

endif
