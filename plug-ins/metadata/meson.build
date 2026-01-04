
plugin_name = 'metadata-editor'

plugin_sourcecode = [
  'metadata-editor.c',
  'metadata-impexp.c',
  'metadata-tags.c',
  'metadata-xml.c',
]
plugin_sources = plugin_sourcecode

if platform_windows
  plugin_rc = configure_file(
    input : gimp_plugins_rc.full_path(),
    output: plugin_name + '.rc',
    copy: true,
  )
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

plugin_exe = executable(plugin_name,
                        plugin_sources,
                        dependencies: [
                          libgimpui_dep,
                          gexiv2,
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

###############################################################################

plugin_name = 'metadata-viewer'

plugin_sourcecode = [
  'metadata-viewer.c',
  'metadata-tag-object.c',
  'metadata-tags.c',
]
plugin_sources = plugin_sourcecode

if platform_windows
  plugin_rc = configure_file(
    input : gimp_plugins_rc.full_path(),
    output: plugin_name + '.rc',
    copy: true,
  )
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

plugin_exe = executable(plugin_name,
                        plugin_sources,
                        include_directories: [ rootInclude, ],
                        dependencies: [
                          gtk3, gegl, gexiv2,
                        ],
                        link_with: [
                          libgimp,
                          libgimpbase,
                          libgimpcolor,
                          libgimpconfig,
                          libgimpmath,
                          libgimpui,
                          libgimpwidgets,
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
