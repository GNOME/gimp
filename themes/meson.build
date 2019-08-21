
themes = [
  'System',
]

foreach theme : themes
  install_data(
    join_paths(theme, 'gimp.css'),
    install_dir: gimpdatadir / 'themes' / theme,
  )
endforeach
