rootAppInclude = include_directories('.')

app_mkenums_custom_target_commonargs = [
  gimp_mkenums_custom_target_commonargs,
  '--dtail','    { 0, NULL, NULL }\n'+
            '  };\n'+
            '\n'+
            '  static GType type = 0;\n'+
            '\n'+
            '  if (G_UNLIKELY (! type))\n'+
            '    {\n'+
            '      type = g_@type@_register_static ("@EnumName@", values);\n'+
            '      gimp_type_set_translation_context (type, "@enumnick@");\n'+
            '      gimp_@type@_set_value_descriptions (type, descs);\n'+
            '    }\n'+
            '\n'+
            '  return type;\n'+
            '}\n',
]

subdir('actions')
subdir('core')
subdir('dialogs')
subdir('display')
subdir('file')
subdir('file-data')
subdir('gegl')
subdir('gui')
subdir('menus')
subdir('operations')
subdir('paint')
subdir('pdb')
subdir('plug-in')
subdir('propgui')
subdir('text')
subdir('tools')
subdir('vectors')
subdir('widgets')
subdir('xcf')


# For app/config
app_debug_files = files(
  'gimp-debug.c',
  'gimp-log.c',
)

# Top-level library

libapp_sources = [
  'app.c',
  'errors.c',
  'gimp-debug.c',
  'gimp-log.c',
  'gimp-update.c',
  'gimp-version.c',
  'language.c',
  'sanity.c',
  'signals.c',
  'tests.c',
  'unique.c',
  gitversion_h,
  gimpdbusservice_gen,
]

libapp = static_library('app',
  libapp_sources,
  include_directories: [ rootInclude, rootAppInclude, configInclude, ],
  c_args: [ '-DG_LOG_DOMAIN="Gimp"', '-DGIMP_APP_GLUE_COMPILATION', ],
  dependencies: [
    gegl, gdk_pixbuf, gtk3,
  ],
)


if platform_windows
  # for GimpDashboard and GimpBacktrace
  psapi_cflags = [ '-DPSAPI_VERSION=1' ]
  link_args = [ '-lpsapi' ]

else
  psapi_cflags = [ ]
  link_args = []
endif

if platform_osx
  link_args += osx_ldflags
endif

libapp_dep = declare_dependency(
  dependencies: [
    dbghelp,
    drmingw,
    gegl,
    gexiv2,
    gio,
    gio_specific,
    gtk3,
    lcms,
    libbacktrace,
    pangocairo,
    pangoft2,
    rpc,
  ],
  link_with: [
    libapp,
    libappcore,
    libappfile,
    libappfiledata,
    libappgegl,
    libappinternalprocs,
    libapplayermodes,
    libapplayermodeslegacy,
    libappoperations,
    libapppaint,
    libapppdb,
    libappplugin,
    libapptext,
    libappvectors,
    libappxcf,
  ],
  include_directories: [
    rootInclude,
    rootAppInclude,
  ],
  compile_args: psapi_cflags,
  link_args: link_args,
)


# Those subdirs need to link against the first ones
subdir('config')
subdir('tests')



app_links = [
  libappconfig,
  libgimpbase,
  libgimpcolor,
  libgimpconfig,
  libgimpmath,
  libgimpmodule,
  libgimpthumb,
]

app_gui_links = [
  libappactions,
  libappdialogs,
  libappdisplay,
  libappgui,
  libappmenus,
  libapppropgui,
  libapptools,
  libappwidgets,
  libgimpwidgets,
]

# Executables

if platform_windows
  console_rc_name = 'gimp-console-'+ gimp_app_version
  gimp_app_console_rc = configure_file(
    input : gimp_plugins_rc,
    output: console_rc_name + '.rc',
    copy: true,
  )
  console_rc_file = windows.compile_resources(
    gimp_app_console_rc,
    args: [
      '--define', 'ORIGINALFILENAME_STR="@0@"'.format(console_rc_name+'.exe'),
      '--define', 'INTERNALNAME_STR="@0@"'    .format(console_rc_name),
      '--define', 'TOP_SRCDIR="@0@"'          .format(meson.source_root()),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )

  gui_rc_name = 'gimp-'+ gimp_app_version
  gimp_app_gui_rc = configure_file(
    input : gimp_plugins_rc,
    output: gui_rc_name + '.rc',
    copy: true,
  )
  gui_rc_file = windows.compile_resources(
    gimp_app_rc,
    args: [
      '--define', 'ORIGINALFILENAME_STR="@0@"'.format(gui_rc_name+'.exe'),
      '--define', 'INTERNALNAME_STR="@0@"'    .format(gui_rc_name),
      '--define', 'TOP_SRCDIR="@0@"'          .format(meson.source_root()),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
else
  console_rc_file = []
  gui_rc_file = []
endif

gimpconsole_exe = executable('gimp-console-'+gimp_app_version,
  'main.c',
  libapp_sources,
  console_rc_file,
  c_args: [
    '-DG_LOG_DOMAIN="Gimp"',
    '-DGIMP_APP_GLUE_COMPILATION',
    '-DGIMP_CONSOLE_COMPILATION',
    psapi_cflags,
  ],
  dependencies: libapp_dep,
  link_with: [
    app_links,
  ],
  install: true,
)

gimpmain_exe = executable('gimp-'+gimp_app_version,
  'main.c',
  libapp_sources,
  gui_rc_file,
  c_args: [
    '-DG_LOG_DOMAIN="Gimp"',
    '-DGIMP_APP_GLUE_COMPILATION',
    psapi_cflags,
  ],
  dependencies: libapp_dep,
  link_with: [
    app_links,
    app_gui_links,
  ],
  install: true,
)
