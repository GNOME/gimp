icons_dir = size + 'x' + size

foreach icon : source_icons
  # Install the color variant.
  icon_name = icon + '.png'
  outdir    = gimpiconsdir / theme / icons_dir / 'apps'
  if icon == 'gimp-wilber'
    input_path = meson.project_source_root() / 'gimp-data/images/logo/gimp-logo-shadow.svg'
  else
    input_path = '../scalable/' + icon + '.svg'
  endif

  custom_target(icon_name,
                input : [ input_path ],
                output: [ icon_name ],
                command: [
                  colorsvg2png, '@INPUT@', '@OUTPUT@', size,
                ],
                build_by_default: true,
                install: true,
                install_dir: outdir)

  # Install the symbolic variant.
  icon_name = icon + '-symbolic.symbolic.png'
  outdir    = gimpiconsdir / theme / icons_dir / 'apps'
  if icon == 'gimp-wilber'
    input_path = meson.project_source_root() / 'gimp-data/images/logo/gimp-wilber-symbolic.svg'
  else
    input_path = '../scalable/' + icon + '-symbolic.svg'
  endif

  custom_target(icon_name,
                input : [ input_path ],
                output: [ icon_name ],
                command: [
                  gtk_encode_symbolic_svg, '-o', '@OUTDIR@', '@INPUT@', icons_dir
                ],
                build_by_default: true,
                install: true,
                install_dir: outdir)
endforeach
