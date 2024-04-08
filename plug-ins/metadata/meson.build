
plugin_name = 'metadata-editor'

plugin_sources = [
  'metadata-editor.c',
  'metadata-impexp.c',
  'metadata-tags.c',
  'metadata-xml.c',
]

if platform_windows
  plugin_rc = configure_file(
    input : gimp_plugins_rc.full_path(),
    output: plugin_name + '.rc',
    copy: true,
  )
  plugin_sources += windows.compile_resources(
    plugin_rc,
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
                          gexiv2,
                        ],
                        install: true,
                        install_dir: gimpplugindir / 'plug-ins' / plugin_name)
plugin_executables += [plugin_exe.full_path()]

###############################################################################

plugin_name = 'metadata-viewer'

plugin_sources = [
  'metadata-viewer.c',
  'metadata-tags.c',
]

if platform_windows
  plugin_rc = configure_file(
    input : gimp_plugins_rc.full_path(),
    output: plugin_name + '.rc',
    copy: true,
  )
  plugin_sources += windows.compile_resources(
    plugin_rc,
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
                        install: true,
                        install_dir: gimpplugindir / 'plug-ins' / plugin_name)
plugin_executables += [plugin_exe.full_path()]
