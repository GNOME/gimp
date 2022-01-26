icons_dir = size + 'x' + size

foreach icon : source_icons
  icon_name = icon + '-symbolic.symbolic.png'
  input_path = '../scalable/' + icon + '-symbolic.svg'

  custom_target(icon_name,
    input : [ input_path ],
    output: [ icon_name ],
    command: [
      gtk_encode_symbolic_svg, '-o', '@OUTDIR@', '@INPUT@', icons_dir
    ],
    install: true,
    install_dir: gimpiconsdir / theme / icons_dir / 'apps',
  )
endforeach
