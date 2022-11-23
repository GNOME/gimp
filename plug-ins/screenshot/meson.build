plugin_name = 'screenshot'

plugin_sources = [
  'screenshot-freedesktop.c',
  'screenshot-osx.c',
  'screenshot-win32.c',
  'screenshot-x11.c',
  'screenshot.c',
]

if platform_windows
  plugin_sources += windows.compile_resources('screenshot-win32.rc')
endif


screenshot = executable('screenshot',
  plugin_sources,
  dependencies: [
    libligmaui_dep,
    x11, xmu, xext, xfixes,
  ],
  install: true,
  install_dir: ligmaplugindir / 'plug-ins' / plugin_name,
)
