
scriptfuInclude = include_directories('..')

plugin_name = 'script-fu-server'

plugin_sourcecode = [
  'script-fu-server.c',
  'script-fu-server-plugin.c',
]
plugin_sources = plugin_sourcecode

if not meson.is_cross_build()
  if platform_windows
    plugin_sources += windows.compile_resources(
      plugin_rc,
      args: [
        rc_define_flag, rc_define_fmt.format('ORIGINALFILENAME_STR', plugin_name+'.exe'),
        rc_define_flag, rc_define_fmt.format('INTERNALNAME_STR', plugin_name),
      ],
      include_directories: [
        rootInclude, appInclude,
      ],
    )
  endif

  # The differences from the script-fu executable:
  #    name and sources
  #    also links with winsock

  # Note the server rolls its own log, no need for a different G_LOG_DOMAIN

  plugin_exe = executable(plugin_name,
                          plugin_sources,
                          dependencies: [
                            libgimpui_dep,
                            math,
                            winsock,
                          ],
                          c_args: [
                            '-DG_LOG_DOMAIN="scriptfu"',
                          ],
                          include_directories: [
                            scriptfuInclude,
                          ],
                          link_with : libscriptfu,
                          win_subsystem: 'windows',
                          install: true,
                          install_dir: gimpplugindir / 'plug-ins' / plugin_name)
endif
