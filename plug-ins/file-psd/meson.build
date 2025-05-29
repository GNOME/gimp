plugin_name = 'file-psd'

plugin_sourcecode = [
  'psd-image-res-load.c',
  'psd-layer-res-load.c',
  'psd-load.c',
  'psd-export.c',
  'psd-thumb-load.c',
  'psd-util.c',
  'psd.c',
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
                        c_args: '-DG_LOG_DOMAIN="file-psd"',
                        dependencies: [
                          libgimpui_dep,
                          libjpeg,
                          math,
                          zlib,
                        ],
                        win_subsystem: 'windows',
                        install: true,
                        install_dir: gimpplugindir / 'plug-ins' / plugin_name)
plugin_executables += [plugin_exe.full_path()]
