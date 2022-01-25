icons_dir = size + 'x' + size

foreach color_icon : source_icons
  split_name = color_icon.split('.')
  if split_name.length() != 2
    error('Unexpected icon name: ' + color_icon)
  endif
  output_path = split_name[0] + '-symbolic.symbolic.' + split_name[-1]
  icon_name = output_path.split('/')[1]

  input_path = split_name[0] + '-symbolic.svg'
  input_path = '../scalable/' + input_path.split('/')[1]

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
