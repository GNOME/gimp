
# Build the one executable that defines five PDB procs.
# The executable is not versioned since installed private in /plug-ins

# Not define include_directories; no higher gimp source references scriptfu

subdir('libscriptfu')
subdir('scripts')

executable_name = 'script-fu'

# script-fu.c registers registers many PDB procs in the PDB.
# Four source files implement the PDB procedures of type PLUGIN, of similar names.
# script-fu.c also implements PDB procedure of type EXTENSION "extension-script-fu"

plugin_sources = [
  'script-fu-console.c',
  'script-fu-eval.c',
  'script-fu-server.c',
  'script-fu-text-console.c',

  'script-fu.c',
]

if platform_windows
  plugin_sources += windows.compile_resources(
    gimp_plugins_rc,
    args: [
      '--define', 'ORIGINALFILENAME_STR="@0@"'.format(executable_name+'.exe'),
      '--define', 'INTERNALNAME_STR="@0@"'    .format(executable_name),
      '--define', 'TOP_SRCDIR="@0@"'          .format(meson.source_root()),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
endif

# Several components use Gtk
# script-fu-server uses sockets

# libscriptfu is installed to standard place; no rpath necessary

executable(executable_name,
  plugin_sources,
  libscriptfu,
  dependencies: [
    libgimpui_dep,
    math,
    winsock,
  ],
  c_args: [
    '-DG_LOG_DOMAIN="scriptfu"',
    '-DSTANDALONE=0',
    '-DUSE_INTERFACE=1',
    '-DUSE_STRLWR=0',
  ],
  include_directories: [
    libscriptfuInclude,
  ],
  link_with : libscriptfu,
  install: true,
  install_dir: gimpplugindir / 'plug-ins' / executable_name,
)
