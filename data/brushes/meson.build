brushes_types = [
  'Basic',
  'gimp-obsolete-files',
  'Legacy',
  'Media',
  'Sketch',
  'Splatters',
  'Texture',
]

foreach brushes_type : brushes_types

  install_subdir(brushes_type,
    install_dir: join_paths(gimpdatadir, 'brushes'),
    exclude_files: [
      '.gitignore',
      'Makefile.am',
      'Makefile.in',
    ],
  )

endforeach
