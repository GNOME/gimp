patterns_types = [
  'Animal',
  'Fabric',
  'Food',
  'Legacy',
  'Paper',
  'Plant',
  'Sky',
  'Stone',
  'Water',
  'Wood',
]

foreach patterns_type : patterns_types

  install_subdir(patterns_type,
    install_dir: join_paths(gimpdatadir, 'patterns'),
    exclude_files: [
      '.gitignore',
      'Makefile.am',
      'Makefile.in',
    ],
  )

endforeach
