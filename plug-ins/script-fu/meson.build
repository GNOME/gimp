
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
if not stable or not release or gimp_version.endswith('+git')
  subdir('test')
endif


plugin_name = 'script-fu'

# script-fu.c registers registers many PDB procs in the PDB.
# Several source files implement the PDB procedures of type PLUGIN, of similar names.
# script-fu.c also implements PDB procedure of type EXTENSION "extension-script-fu"

plugin_sourcecode = [
  'script-fu-eval.c',
  'script-fu-text-console.c',
  'script-fu-refresh.c',

  'script-fu.c',
]
plugin_sources = plugin_sourcecode

if not meson.is_cross_build()
  if platform_windows
    plugin_sources += windows.compile_resources(
      plugin_rc,
      args: [
        rc_define_flag, rc_define_fmt.format('ORIGINALFILENAME_STR', plugin_name+'.exe'),
        rc_define_flag, rc_define_fmt.format('INTERNALNAME_STR', plugin_name),
      ],
      include_directories: [
        rootInclude, appInclude,
      ],
    )

    #.interp file read by gimp_interpreter_db_load_interp_file in gimpinterpreterdb.c
    scriptfu_config = configuration_data()
    scriptfu_config.set('SCRIPTFU_PATH', '')
    configure_file(
      input : 'gimp-script-fu-interpreter.interp.in',
      output: 'gimp-script-fu-interpreter.interp',
      configuration: scriptfu_config,
      install: true,
      install_dir: gimpplugindir / 'interpreters',
    )

    configure_file(
      input : 'gimp-script-fu-interpreter.interp.in',
      output: 'gimp-script-fu-interpreter_win.interp',
      configuration: scriptfu_config,
      install: true,
      install_dir: gimpplugindir / 'interpreters',
    )
  endif

  # Several components use Gtk

  # libscriptfu is installed to standard place; no rpath necessary

  plugin_exe = executable(plugin_name,
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
                          win_subsystem: 'windows',
                          install: true,
                          install_dir: gimpplugindir / 'plug-ins' / plugin_name)

  # Ugly trick to add executables to plugin_custom_targets list (see root meson.build)
  plugin_custom_targets += custom_target('dummy-' + plugin_name,
                input: plugin_exe,
                output: 'dummy-' + plugin_name + '.dummy',
                command: [python, '-c',
                          'open("@OUTPUT@", "w").write("Nice. I was built before calling in-build-gimp.py\\n")'],
                build_by_default: true)
  plugin_executables += [plugin_exe.full_path()]
endif
