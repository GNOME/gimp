
# Build executable plugins that define several PDB procs.
# An executable is not versioned since installed private in /plug-ins

# Not define include_directories; no higher gimp source references scriptfu

# Not using c_args: -DSTANDALONE=0','-DUSE_INTERFACE=1','-DUSE_STRLWR=0',
# since those are compile time switches for tinyscheme, not present in this dir

subdir('libscriptfu')
subdir('scripts')
subdir('server')
subdir('interpreter')
subdir('console')
if not stable or not release
  subdir('test')
endif


executable_name = 'script-fu'

# script-fu.c registers registers many PDB procs in the PDB.
# Several source files implement the PDB procedures of type PLUGIN, of similar names.
# script-fu.c also implements PDB procedure of type EXTENSION "extension-script-fu"

plugin_sources = [
  'script-fu-eval.c',
  'script-fu-text-console.c',
  'script-fu-refresh.c',

  'script-fu.c',
]

if not meson.is_cross_build()
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

    # Fallback fix to the problem of non-configured interpreters
    scriptfu_config = configuration_data()
    scriptfu_config.set('SCRIPTFU_PATH', '')
    configure_file(
      input : 'gimp-script-fu-interpreter.interp.in',
      output: 'gimp-script-fu-interpreter.interp',
      configuration: scriptfu_config,
      install: true,
      install_dir: gimpplugindir / 'interpreters',
    )
  endif

  # Several components use Gtk

  # libscriptfu is installed to standard place; no rpath necessary

  plugin_exe = executable(executable_name,
                          plugin_sources,
                          dependencies: [
                            libgimpui_dep,
                            math,
                            gi,
                          ],
                          c_args: [
                            '-DG_LOG_DOMAIN="scriptfu"',
                          ],
                          include_directories: [
                            libscriptfuInclude,
                          ],
                          link_with : [libscriptfuconsole, libscriptfu ],
                          install: true,
                          install_dir: gimpplugindir / 'plug-ins' / executable_name)
  plugin_executables += [plugin_exe.full_path()]
endif
