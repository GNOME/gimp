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
  subdir(patterns_type)
  install_data(
    patterns,
    install_dir: gimpdatadir / 'patterns' / patterns_type,
  )
endforeach
