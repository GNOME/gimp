
scriptfuInclude = include_directories('..')

executable_name = 'gimp-script-fu-interpreter' + exec_ver

plugin_sources = [
  'script-fu-interpreter.c',
  'script-fu-interpreter-plugin.c',
]

if platform_windows
  plugin_sources += windows.compile_resources(
    plugin_rc,
    args: [
      '--define', 'ORIGINALFILENAME_STR="@0@"'.format(executable_name+'.exe'),
      '--define', 'INTERNALNAME_STR="@0@"'    .format(executable_name),
      '--define', 'TOP_SRCDIR="@0@"'          .format(meson.project_source_root()),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
endif

# !!! Installs as a usual binary say to /usr/bin, unlike extension-script-fu
# GIMP queries scripts with shebangs, which invokes gimp-script-fu-interpreter-3.0.

script_fu_interpreter = executable(executable_name,
  plugin_sources,
  dependencies: [
    libgimpui_dep,
    math,
  ],
  c_args: [
    '-DG_LOG_DOMAIN="scriptfu"',
  ],
  include_directories: [
    scriptfuInclude,
  ],
  link_with : libscriptfu,
  install: true,
)

if enable_default_bin and meson.version().version_compare('>=0.61.0')
  install_symlink(fs.name(script_fu_interpreter.full_path()).replace(exec_ver, ''),
    pointing_to: fs.name(script_fu_interpreter.full_path()),
    install_dir: get_option('bindir')
  )
endif
