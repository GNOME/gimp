plugins_subdirs = [
  'colorxhtml',
  'file-openraster',
  'foggify',
  'gradients-save-as-css',
  'histogram-export',
  'palette-offset',
  'palette-sort',
  'palette-to-gradient',
  'py-slice',
  'python-eval',
  'spyro_plus',
]

if not stable
  plugins_subdirs += [
    'benchmark-foreground-extract',
    'clothify',
    'shadow_bevel',
    'sphere',
    'whirlpinch',
  ]
endif


install_subdir(plugins_subdirs,
  install_dir: gimpplugindir / 'plug-ins' / plugin_name,
)
