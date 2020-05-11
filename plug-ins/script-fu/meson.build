scriptfu_incdir = include_directories('.')

subdir('tinyscheme')
subdir('ftx')
subdir('scripts')

plugin_name = 'script-fu'

plugin_sources = [
  'scheme-wrapper.c',
  'script-fu-console.c',
  'script-fu-eval.c',
  'script-fu-interface.c',
  'script-fu-regex.c',
  'script-fu-script.c',
  'script-fu-scripts.c',
  'script-fu-server.c',
  'script-fu-text-console.c',
  'script-fu-utils.c',
  'script-fu.c',
]

if platform_windows
  plugin_sources += windows.compile_resources(
    gimp_plugins_rc,
    args: [
      '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plugin_name+'.exe'),
      '--define', 'INTERNALNAME_STR="@0@"'    .format(plugin_name),
      '--define', 'TOP_SRCDIR="@0@"'          .format(meson.source_root()),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
endif

executable(plugin_name,
  plugin_sources,
  dependencies: [
    libgimpui_dep,
    math,
    winsock,
  ],
  c_args: [
    '-DSTANDALONE=0',
    '-DUSE_INTERFACE=1',
    '-DUSE_STRLWR=0',
  ],
  link_whole: [
    scriptfu_tinyscheme,
    scriptfu_ftx,
  ],
  install: true,
  install_dir: gimpplugindir / 'plug-ins' / plugin_name,
)
