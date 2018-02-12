
themes = [
  # 'Dark',
  # 'Gray',
  # 'Light',
  'Small',
  'System',
]

foreach theme : themes
  install_data(
    join_paths(theme, 'gimp.css'),
    install_dir: join_paths(gimpdatadir, 'themes', theme)
  )

  if run_command('[', '-d', join_paths(theme, 'ui'), ']').returncode() == 0
    install_subdir(
      join_paths(theme, 'ui'),
      install_dir: join_paths(gimpdatadir, 'themes', theme)
    )
  endif
endforeach
