if have_screenshot

plugin_screenshot_sources = [
  'screenshot-freedesktop.c',
  'screenshot-gnome-shell.c',
  'screenshot-kwin.c',
  'screenshot-osx.c',
  'screenshot-win32.c',
  'screenshot-x11.c',
  'screenshot.c',
  platform_win32 ? windows.compile_resources('screenshot-win32.rc') : [],
]

screenshot = executable('screenshot',
  plugin_screenshot_sources,
  include_directories: [ rootInclude, ],
  dependencies: [
    gtk2, gegl,
    x11, xmu, xext, xfixes,
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
