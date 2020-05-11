# C version

plugin_name = 'goat-exercise'

plugin_sources = [
  'goat-exercise-c.c',
]

if platform_windows
  plugin_sources += windows.compile_resources(
    gimp_plugins_rc,
    args: [
      '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plugin_name + '-c.exe'),
      '--define', 'INTERNALNAME_STR="@0@"'    .format(plugin_name),
      '--define', 'TOP_SRCDIR="@0@"'          .format(meson.source_root()),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
endif

executable(plugin_name + '-c',
  plugin_sources,
  dependencies: [
    libgimpui_dep,
    math,
  ],
  install: true,
  install_dir: gimpplugindir /  'plug-ins' / plugin_name + '-c',
)

install_data(
  'goat-exercise-c.c',
  install_dir: gimpplugindir /  'plug-ins' / plugin_name + '-c',
)

# Python 3 (pygobject) version.

if have_python
  install_data(
    'goat-exercise-py3.py',
    install_dir: gimpplugindir /  'plug-ins' / plugin_name + '-py3',
  )
endif

# Javascript (GJS) version.

if have_javascript
  install_data(
    'goat-exercise-gjs.js',
    install_dir: gimpplugindir /  'plug-ins' / plugin_name + '-gjs',
  )
endif

# Lua (lua-jit + LGI) version.

if have_lua
  install_data(
    'goat-exercise-lua.lua',
    install_dir: gimpplugindir /  'plug-ins' / plugin_name + '-lua',
  )
endif
