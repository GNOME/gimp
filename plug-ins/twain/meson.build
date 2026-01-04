if not platform_windows or host_cpu_family != 'x86'
  subdir_done()
endif

plugin_name = 'twain'

plugin_sourcecode = [
  'tw_func.c',
  'tw_util.c',
  'tw_win.c',
  'twain.c',
]
plugin_sources = plugin_sourcecode

plugin_sources += windows.compile_resources(
  gimp_plugins_rc,
  args: [
    rc_define_flag, rc_define_fmt.format('ORIGINALFILENAME_STR', plugin_name+'.exe'),
    rc_define_flag, rc_define_fmt.format('INTERNALNAME_STR', plugin_name),
  ],
  include_directories: [
    rootInclude, appInclude,
  ],
)

twain = executable(plugin_name,
                   plugin_sources,
                   dependencies: [
                     libgimp_dep,
                     glib,
                     gtk3,
                   ],
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
plugin_executables += [twain.full_path()]
