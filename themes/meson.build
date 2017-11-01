
theme_names = [
  [ 'Lighter',  '00-Lighter' ],
  [ 'Light',    '01-Light' ],
  [ 'Gray',     '02-Gray' ],
  [ 'Dark',     '03-Dark' ],
  [ 'Darker',   '04-Darker' ],
  [ 'System',   'System' ],
]

foreach theme : theme_names
  theme_srcdir = theme[0]
  theme_bindir = theme[1]
  install_data(
    join_paths(theme_srcdir, 'gtkrc'),
    install_dir: join_paths(gimpdatadir, 'themes', theme_bindir)
  )

  if run_command('[', '-d', join_paths(theme_srcdir, 'ui'), ']').returncode() == 0
    install_subdir(
      join_paths(theme_srcdir, 'ui'),
      install_dir: join_paths(gimpdatadir, 'themes', theme_bindir)
    )
  endif
endforeach
