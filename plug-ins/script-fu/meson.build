if have_scriptfu

scriptfu_incdir = include_directories('.')

subdir('tinyscheme')
subdir('ftx')
subdir('scripts')

plugin_name = 'script-fu'

rc_file = platform_win32 ? windows.compile_resources(
  gimp_plugins_rc,
  args: [
    '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plugin_name+'.exe'),
    '--define', 'INTERNALNAME_STR="@0@"'    .format(plugin_name),
    '--define', 'TOP_SRCDIR="@0@"'          .format(meson.source_root()),
  ],
  include_directories: [
    rootInclude, appInclude,
  ],
) : []

plugin_scriptfu_sources = [
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
  rc_file,
]

winsock = platform_win32 ? cc.find_library('ws2_32') : []

executable(plugin_name,
  plugin_scriptfu_sources,
  include_directories: [ rootInclude, ],
  dependencies: [
    gegl, gtk3, math, winsock,
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
  install_dir: join_paths(gimpplugindir, 'plug-ins'),
)

endif
