
# Build executable plugins that define several PDB procs.
# An executable is not versioned since installed private in /plug-ins

# Not define include_directories; no higher gimp source references scriptfu

# Not using c_args: -DSTANDALONE=0','-DUSE_INTERFACE=1','-DUSE_STRLWR=0',
# since those are compile time switches for tinyscheme, not present in this dir

subdir('libscriptfu')
subdir('scripts')
subdir('server')
subdir('interpreter')


executable_name = 'script-fu'

# script-fu.c registers registers many PDB procs in the PDB.
# Several source files implement the PDB procedures of type PLUGIN, of similar names.
# script-fu.c also implements PDB procedure of type EXTENSION "extension-script-fu"

plugin_sources = [
  'script-fu-console.c',
  'script-fu-eval.c',
  'script-fu-text-console.c',

  'script-fu.c',
]

if platform_windows
  plugin_sources += windows.compile_resources(
    plugin_rc,
    args: [
      '--define', 'ORIGINALFILENAME_STR="@0@"'.format(executable_name+'.exe'),
      '--define', 'INTERNALNAME_STR="@0@"'    .format(executable_name),
      '--define', 'TOP_SRCDIR="@0@"'          .format(meson.project_source_root()),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
endif

# Several components use Gtk

# libscriptfu is installed to standard place; no rpath necessary

executable(executable_name,
  plugin_sources,
  dependencies: [
    libgimpui_dep,
    math,
  ],
  c_args: [
    '-DG_LOG_DOMAIN="scriptfu"',
  ],
  include_directories: [
    libscriptfuInclude,
  ],
  link_with : libscriptfu,
  install: true,
  install_dir: gimpplugindir / 'plug-ins' / executable_name,
)
