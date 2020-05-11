if have_screenshot

plugin_sources = [
  'screenshot-freedesktop.c',
  'screenshot-gnome-shell.c',
  'screenshot-kwin.c',
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
    libgimpui_dep,
    x11, xmu, xext, xfixes,
  ],
  install: true,
  install_dir: gimpplugindir / 'plug-ins' / plugin_name,
)

endif
