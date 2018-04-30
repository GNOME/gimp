rootAppInclude = include_directories('.')

app_mkenums_custom_target_command = [
  gimp_mkenums,
  '--fhead','#include "config.h"\n'
          + '#include <gio/gio.h>\n'
          + '#include "libgimpbase/gimpbase.h"\n'
          + '#include "core/core-enums.h"\n'
          + '#include "@INPUT@"\n'
          + '#include "gimp-intl.h"\n',
  gimp_mkenums_custom_target_commonargs,
]

subdir('actions')
subdir('core')
subdir('dialogs')
subdir('display')
subdir('file')
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


# Those subdirs need to link againts the first ones
subdir('config')
subdir('tests')



app_links = [
  libappconfig,
  libappcore,
  libappfile,
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

app_deps = [
  gio, gtk3, pangocairo, pangoft2, gegl, lcms, gexiv2, drmingw,
]

# Executables

rc_name = 'gimp-'+ app_version
rc_file = platform_win32 ? windows.compile_resources(
  gimp_app_rc,
  args: [
    '--define', 'ORIGINALFILENAME_STR="@0@"'.format(rc_name+'.exe'),
    '--define', 'INTERNALNAME_STR="@0@"'    .format(rc_name),
    '--define', 'TOP_SRCDIR="@0@"'          .format(meson.source_root()),
  ],
  include_directories: [
    rootInclude, appInclude,
  ],
) : []

gimpconsole_exe = executable('gimp-console-'+app_version,
  libapp_sources + [ 'main.c', rc_file, ],
  c_args: [ '-DG_LOG_DOMAIN="Gimp"', '-DGIMP_APP_GLUE_COMPILATION', ] + [ '-DGIMP_CONSOLE_COMPILATION', ],
  include_directories: [ rootInclude, ],
  dependencies: app_deps,
  link_with: app_links,
  install: true,
)

rc_name = 'gimp-console-'+ app_version
rc_file = platform_win32 ? windows.compile_resources(
  gimp_app_rc,
  args: [
    '--define', 'ORIGINALFILENAME_STR="@0@"'.format(rc_name+'.exe'),
    '--define', 'INTERNALNAME_STR="@0@"'    .format(rc_name),
    '--define', 'TOP_SRCDIR="@0@"'          .format(meson.source_root()),
  ],
  include_directories: [
    rootInclude, appInclude,
  ],
) : []

gimpmain_exe = executable('gimp-'+app_version,
  libapp_sources + [ 'main.c', rc_file, ],
  c_args: [ '-DG_LOG_DOMAIN="Gimp"', '-DGIMP_APP_GLUE_COMPILATION', ],
  include_directories: [ rootInclude, ],
  dependencies: app_deps,
  link_with: app_links + app_gui_links,
  install: true,
)
