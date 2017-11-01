brushes_types = [
  'Basic',
  'Fun',
  'Legacy',
  'Media',
  'Sketch',
  'Splatters',
  'Texture',
]

foreach brushes_type : brushes_types
  subdir(brushes_type)
  install_data(
    brushes,
    install_dir: gimpdatadir / 'brushes' / brushes_type,
  )
endforeach
