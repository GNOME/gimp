# C version

extension_name = 'org.gimp.extension.goat-exercises'
plug_in_name   = 'goat-exercise'

plugin_sources = [
  'goat-exercise-c.c',
]

if platform_windows
  plugin_sources += windows.compile_resources(
    gimp_plugins_rc,
    args: [
      '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plug_in_name + '-c.exe'),
      '--define', 'INTERNALNAME_STR="@0@"'    .format(plug_in_name),
      '--define', 'TOP_SRCDIR="@0@"'          .format(meson.project_source_root()),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
endif

exe = executable(plug_in_name + '-c',
  plugin_sources,
  dependencies: [
    libgimpui_dep,
    math,
  ],
  install: true,
  install_dir: gimpplugindir /  'extensions' / extension_name,
)

# XXX This is so ugly!
# From meson 0.54.0, we will be able to use exe.name().
plug_ins = exe.full_path().split('/')[-1].split('\\')[-1]

install_data(
  'goat-exercise-c.c',
  install_dir: gimpplugindir /  'extensions' / extension_name,
)

# Vala version

if have_vala and have_gobject_introspection
  exe = executable('goat-exercise-vala',
    'goat-exercise-vala.vala',
    include_directories: [ rootInclude, ],
    dependencies: [
      libgimp_vapi, libgimpui_vapi, gtk3, gegl, math,
    ],
    c_args: [
      '-DGETTEXT_PACKAGE="@0@"'.format('org.gimp.extension.goat-exercises'),
    ],
    install: true,
    install_dir: gimpplugindir /  'extensions' / extension_name,
  )
  plug_ins = plug_ins + ':' + exe.full_path().split('/')[-1].split('\\')[-1]

  install_data(
    'goat-exercise-vala.vala',
    install_dir: gimpplugindir /  'extensions' / extension_name,
  )
endif

# Python 3 (pygobject) version.

install_data(
  'goat-exercise-py3.py',
  install_dir: gimpplugindir /  'extensions' / extension_name,
)
plug_ins = plug_ins + ':goat-exercise-py3.py'

# Javascript (GJS) version.

if have_javascript
  install_data(
    'goat-exercise-gjs.js',
    install_dir: gimpplugindir /  'extensions' / extension_name,
  )
  plug_ins = plug_ins + ':goat-exercise-gjs.js'
endif

# Lua (lua-jit + LGI) version.

if have_lua
  install_data(
    'goat-exercise-lua.lua',
    install_dir: gimpplugindir /  'extensions' / extension_name,
  )
  plug_ins = plug_ins + ':goat-exercise-lua.lua'
endif

# Generate the AppData.

conf = configuration_data()
conf.set('GOAT_EXERCISES', plug_ins)

appdatafilename = 'org.gimp.extension.goat-exercises.metainfo.xml'
appdatafilein = configure_file(
  input : appdatafilename + '.in.in',
  output: appdatafilename + '.in',
  configuration: conf,
)

appdatafile = i18n.merge_file(
  input : [ appdatafilein, ],
  output: appdatafilename,
  po_dir: po_plug_ins_dir,
  install: true,
  install_dir: gimpplugindir /  'extensions' / extension_name,
)
