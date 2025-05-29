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
    '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plugin_name+'.exe'),
    '--define', 'INTERNALNAME_STR="@0@"'    .format(plugin_name),
    '--define', 'TOP_SRCDIR="@0@"'          .format(meson.project_source_root()),
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
plugin_executables += [twain.full_path()]
