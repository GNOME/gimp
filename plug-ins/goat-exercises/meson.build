plugin_name = 'goat-exercise'

plugin_sources = [
  'goat-exercise-c.c',
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
  include_directories: [ rootInclude, ],
  dependencies: [
    gtk3, gegl, math,
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
  install_dir: gimpplugindir /  'plug-ins' / plugin_name,
)


install_data(
  'goat-exercise-c.c',
  install_dir: gimpplugindir /  'plug-ins' / plugin_name + '-c',
)

if have_python
  install_data(
    'goat-exercise-py3.py',
    install_dir: gimpplugindir /  'plug-ins' / plugin_name + '-py3',
  )
endif


if have_javascript
  install_data(
    'goat-exercise-gjs.js',
    install_dir: gimpplugindir /  'plug-ins' / plugin_name + '-gjs',
  )
endif

if have_lua
  install_data(
    'goat-exercise-lua.lua',
    install_dir: gimpplugindir /  'plug-ins' / plugin_name + '-lua',
  )
endif
