plugins = [
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
  plugins += [
    'benchmark-foreground-extract',
    'clothify',
    'shadow_bevel',
    'sphere',
    'whirlpinch',
  ]
endif

foreach plugin : plugins
  name = plugin
  srcs = name + '.py'

  install_data(srcs, install_dir: gimpplugindir / name)
endforeach
