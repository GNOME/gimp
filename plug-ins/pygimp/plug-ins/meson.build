plugins_scripts = [
  'colorxhtml.py',
  'file-openraster.py',
  'foggify.py',
  'gradients-save-as-css.py',
  'histogram-export.py',
  'palette-offset.py',
  'palette-sort.py',
  'palette-to-gradient.py',
  'py-slice.py',
  'python-eval.py',
]

if not stable
  plugins_scripts += [
    'benchmark-foreground-extract.py',
    'clothify.py',
    'shadow_bevel.py',
    'sphere.py',
    'whirlpinch.py',
  ]
endif


install_data(plugins_scripts,
  install_dir: join_paths(gimpplugindir, 'plug-ins', plugin_name),
)
install_data([
    'pyconsole.py',
    'python-console.py',
  ],
  install_dir: join_paths(gimpplugindir, 'plug-ins', 'python-console'),
)
