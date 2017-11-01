tools_presets_types = [
  'Crop',
  'FX',
  'Paint',
  'Selection',
  'Sketch',
]

foreach tools_presets_type : tools_presets_types

  install_subdir(tools_presets_type,
    install_dir: join_paths(gimpdatadir, 'tools-presets'),
    exclude_files: [
      '.gitignore',
      'Makefile.am',
      'Makefile.in',
    ],
  )

endforeach
