subdir('Brushes')
subdir('Paper')
subdir('Presets')

plugin_name = 'ligmaressionist'

plugin_sources = [
  'brush.c',
  'color.c',
  'general.c',
  'ligma.c',
  'ligmaressionist.c',
  'globals.c',
  'orientation.c',
  'orientmap.c',
  'paper.c',
  'placement.c',
  'plasma.c',
  'ppmtool.c',
  'presets.c',
  'preview.c',
  'repaint.c',
  'size.c',
  'sizemap.c',
  'utils.c',
]

if platform_windows
  plugin_sources += windows.compile_resources(
    ligma_plugins_rc,
    args: [
      '--define', 'ORIGINALFILENAME_STR="@0@"'.format(plugin_name+'.exe'),
      '--define', 'INTERNALNAME_STR="@0@"'    .format(plugin_name),
      '--define', 'TOP_SRCDIR="@0@"'          .format(meson.project_source_root()),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
endif

executable(plugin_name,
  plugin_sources,
  dependencies: [
    libligmaui_dep,
    math,
  ],
  c_args: [
    '-DDEFAULTPATH="@0@:@1@"'.format(
      '~/'   / ligmadir      / 'ligmaressionist',
      prefix / ligmadatadir  / 'ligmaressionist'
    ),
  ],
  install: true,
  install_dir: ligmaplugindir / 'plug-ins' / plugin_name,
)
