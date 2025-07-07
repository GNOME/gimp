rootAppInclude = include_directories('.')

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
subdir('path')
subdir('pdb')
subdir('plug-in')
subdir('propgui')
subdir('text')
subdir('tools')
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
  'gimpcoreapp.c',
  'gimpconsoleapp.c',
  'gimp-debug.c',
  'gimp-log.c',
  'gimp-update.c',
  'gimp-version.c',
  'language.c',
  'sanity.c',
  'signals.c',
  'unique.c',
  gitversion_h,
  gimpdbusservice_gen,
]

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

libapp_c_args = [
  '-DG_LOG_DOMAIN="Gimp"',
  '-DGIMP_APP_GLUE_COMPILATION',
  psapi_cflags,
]

# Put GIMP core in a lib so we can conveniently link against that in test cases
# and main GIMP executable. Note that we should not use this static library for
# the console executable as it uses different macro flags.
libapp = static_library('app',
  libapp_sources,
  include_directories: [ rootInclude, rootAppInclude, configInclude, ],
  c_args: libapp_c_args,
  dependencies: [ gdk_pixbuf, gegl, gexiv2, gtk3, ],
)

gimpconsole_deps = [
  cairo,
  dbghelp,
  drmingw,
  gdk_pixbuf,
  gegl,
  gexiv2,
  gio,
  gio_specific,
  lcms,
  libbacktrace,
  pangocairo,
  pangoft2,
  rpc,
]

console_libapps = [
    libappcore,
    libappfile,
    libappfiledata,
    libappgegl,
    libappinternalprocs,
    libapplayermodes,
    libapplayermodeslegacy,
    libappoperations,
    libapppaint,
    libapppath,
    libapppdb,
    libappplugin,
    libapptext,
    libappxcf,
]

libapp_dep = declare_dependency(
  dependencies: [
    gimpconsole_deps,
    gtk3,
  ],
  link_with: [
    libapp,
    console_libapps,
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

console_libapps += [ libappconfig ]

app_libgimps = [
  libgimpbase,
  libgimpcolor,
  libgimpconfig,
  libgimpmath,
  libgimpmodule,
  libgimpthumb,
]

gui_libapps = [
  console_libapps,
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
    input : gimp_plugins_rc.full_path(),
    output: console_rc_name + '.rc',
    copy: true,
  )
  console_rc_file = windows.compile_resources(
    gimp_app_console_rc,
    args: [
      '--define', 'ORIGINALFILENAME_STR="@0@"'.format(console_rc_name+'.exe'),
      '--define', 'INTERNALNAME_STR="@0@"'    .format(console_rc_name),
      '--define', 'TOP_SRCDIR="@0@"'          .format(meson.project_source_root()),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )

  gui_rc_name = 'gimp-'+ gimp_app_version
  gimp_app_gui_rc = configure_file(
    input : gimp_plugins_rc.full_path(),
    output: gui_rc_name + '.rc',
    copy: true,
  )
  gui_rc_file = windows.compile_resources(
    gimp_app_rc,
    args: [
      '--define', 'ORIGINALFILENAME_STR="@0@"'.format(gui_rc_name+'.exe'),
      '--define', 'INTERNALNAME_STR="@0@"'    .format(gui_rc_name),
      '--define', 'TOP_SRCDIR="@0@"'          .format(meson.project_source_root()),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
else
  console_rc_file = []
  gui_rc_file = []
endif

if enable_console_bin
  gimpconsole_exe = executable(gimpconsole_exe_name,
    'main.c',
    libapp_sources,
    console_rc_file,
    c_args: [ libapp_c_args, '-DGIMP_CONSOLE_COMPILATION', ],
    dependencies: gimpconsole_deps,
    link_with: [
      app_libgimps,
      console_libapps
    ],
    include_directories: [
      rootInclude,
      rootAppInclude,
    ],
    win_subsystem: 'console',
    link_args: link_args,
    install: true,
  )
endif

gimpmain_exe = executable(gimpmain_exe_name,
  'main.c',
  gui_rc_file,
  c_args: libapp_c_args,
  dependencies: libapp_dep,
  link_with: [
    app_libgimps,
    gui_libapps,
  ],
  win_subsystem: 'windows',
  install: true,
)

if enable_default_bin
  if not platform_windows
    install_symlink(fs.name(gimpmain_exe.full_path()).replace(exec_ver, ''),
                    pointing_to: fs.name(gimpmain_exe.full_path()),
                    install_dir: get_option('bindir'))
    install_symlink(fs.name(gimpmain_exe.full_path()).replace(exec_ver, '-@0@'.format(api_version_major)),
                    pointing_to: fs.name(gimpmain_exe.full_path()),
                    install_dir: get_option('bindir'))
  else
    meson.add_install_script(python, '-c', 'from shutil import copy2; from pathlib import Path; from sys import argv; import os; copy2(argv[1], str(Path(os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")) / "bin" / argv[2]))',
                             gimpmain_exe.full_path(), fs.name(gimpmain_exe.name()).replace(exec_ver, '.exe'))
    meson.add_install_script(python, '-c', 'from shutil import copy2; from pathlib import Path; from sys import argv; import os; copy2(argv[1], str(Path(os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")) / "bin" / argv[2].replace("@0@", argv[3])))',
                             gimpmain_exe.full_path(), fs.name(gimpmain_exe.name()).replace(exec_ver, '-@0@.exe'), api_version_major.to_string())
  endif
  if enable_console_bin
    if not platform_windows
      install_symlink(fs.name(gimpconsole_exe.full_path()).replace(exec_ver, ''),
                      pointing_to: fs.name(gimpconsole_exe.full_path()),
                      install_dir: get_option('bindir'))
      install_symlink(fs.name(gimpconsole_exe.full_path()).replace(exec_ver, '-@0@'.format(api_version_major)),
                      pointing_to: fs.name(gimpconsole_exe.full_path()),
                      install_dir: get_option('bindir'))
    else
      meson.add_install_script(python, '-c', 'from shutil import copy2; from pathlib import Path; from sys import argv; import os; copy2(argv[1], str(Path(os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")) / "bin" / argv[2]))',
                               gimpconsole_exe.full_path(), fs.name(gimpconsole_exe.name()).replace(exec_ver, '.exe'))
      meson.add_install_script(python, '-c', 'from shutil import copy2; from pathlib import Path; from sys import argv; import os; copy2(argv[1], str(Path(os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")) / "bin" / argv[2].replace("@0@", argv[3])))',
                               gimpconsole_exe.full_path(), fs.name(gimpconsole_exe.name()).replace(exec_ver, '-@0@.exe'), api_version_major.to_string())
    endif
  endif
endif
