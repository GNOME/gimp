subdir('Default')

themes = [
  'System',
]

foreach theme : themes
  install_data(
    theme / 'gimp.css',
    install_dir: gimpdatadir / 'themes' / theme,
  )
endforeach
