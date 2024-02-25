install_data('index.theme',
  install_dir: gimpiconsdir / theme,
)

generic_icons = [
  [ 'scalable/actions/', 'scalable/actions/switch-on-symbolic.svg' ],
  [ 'scalable/actions/', 'scalable/actions/switch-off-symbolic.svg' ],
  [ 'scalable/status/',  'scalable/status/software-update-available-symbolic.svg' ]
]

foreach icon_info : generic_icons
  icons_dir = icon_info[0]
  icons_list= icon_info[1]
  install_data(icons_list,
    install_dir: gimpiconsdir / theme / icons_dir,
  )
endforeach
