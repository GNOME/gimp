icons_dir = size + 'x' + size

foreach icon : source_icons
  icon_name = icon + '.png'
  input_path = '../scalable/' + icon + '.svg'

  custom_target(icon_name,
    input : [ input_path ],
    output: [ icon_name ],
    command: [
      colorsvg2png, '@INPUT@', '@OUTPUT@', size,
    ],
    install: true,
    install_dir: gimpiconsdir / theme / icons_dir / 'apps',
  )
endforeach

