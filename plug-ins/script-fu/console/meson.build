# library for script-fu-console
#
# Contains just the run_func for ScriptFu console plugin.
# The plugin is a PDB procedure of type temporary.
# The PDB procedure is registered by script-fu plugin,
# which passes the run_func as a closure at registration time.
#
# Static library: just an archive of object files.
# The library is not installed,
# only linked with the script-fu plugin which references it.

# uses libscriptfu

libscriptfuconsoleInclude = include_directories('.')

libscriptfuconsole_sources = [
  'script-fu-console.c',
  'script-fu-console-editor.c',
  'script-fu-console-history.c',
  'script-fu-console-total.c',
]

libscriptfuconsole = static_library('scriptfu-console',
  libscriptfuconsole_sources,
  include_directories: [
    libscriptfuInclude,
    rootInclude,
    appInclude,
  ],
  c_args: [
    '-DG_LOG_DOMAIN="scriptfu-console"',
  ],
  dependencies: [
    libgimpui_dep,
    math,
  ],
  link_with: libscriptfu,
  install: false,
)
