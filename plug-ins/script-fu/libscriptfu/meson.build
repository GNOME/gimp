libscriptfuInclude = include_directories('.')

subdir('tinyscheme')
subdir('ftx')

libscriptfu_sources = [
  'scheme-wrapper.c',
  'scheme-marshal.c',
  'scheme-marshal-return.c',
  'script-fu-interface.c',
  'script-fu-regex.c',
  'script-fu-script.c',
  'script-fu-scripts.c',
  'script-fu-utils.c',
  'script-fu-errors.c',
  'script-fu-compat.c',
  'script-fu-lib.c',
  'script-fu-proc-factory.c',
  'script-fu-arg.c',
  'script-fu-register.c',
  'script-fu-dialog.c',
  'script-fu-run-func.c',
  'script-fu-command.c',
  'script-fu-version.c',
  'script-fu-widgets-custom.c',
  'script-fu-color.c',
  'script-fu-resource.c',
  'script-fu-progress.c',
]

# !! just "library(...)" which means shared versus static depends on configuration of project.
# Meson defaults to shared, but you can reconfigure to static.
# This library is not generally useful except by core GIMP developers.

# Dependencies:
# libscriptfu uses Gtk (which libgimpui_dep references)
# FUTURE: libscriptfu should use libgimpui but not Gtk directly
# libscriptfu does not use sockets (unlike the outer script-fu or script-fu-server)

# link_whole means the entire ftx and tinyscheme static libraries are in
# this library, whether or not they are used (see meson docs.)

# FUTURE: install private to gimp, in 'lib' subdir parallel to 'modules' subdir
# Not doing this because it complicates packaging
# Instead, this library installs in same place as libgimp
# install_dir: gimpplugindir / 'lib',

libscriptfu = library('gimp-scriptfu-'+ gimp_api_version,
  libscriptfu_sources,
  include_directories: [
    rootInclude,
    appInclude,
  ],
  c_args: [
    '-DG_LOG_DOMAIN="scriptfu"',
    '-DSTANDALONE=0',
    '-DUSE_INTERFACE=1',
    '-DUSE_STRLWR=0',
  ],
  dependencies: [
    libgimpui_dep,
    math,
    gi,
  ],
  link_whole: [
    scriptfu_tinyscheme,
    scriptfu_ftx,
  ],
  vs_module_defs: 'script-fu.def',
  version: so_version,
  install: true,
)
