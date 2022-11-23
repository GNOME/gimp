subdir('Default')
subdir('Compact')

themes = [
  'Gray',
  'System',
]

foreach theme : themes
  install_data(
    theme / 'ligma.css',
    install_dir: ligmadatadir / 'themes' / theme,
  )
endforeach
