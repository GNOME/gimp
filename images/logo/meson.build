docs_logo = stable ? 'gimp-logo' : 'gimp-devel-logo'

logos = [ 'gimp-logo' ]
if not stable
  logos += 'gimp-devel-logo'
endif

pngs = []
foreach logo: logos
  logo_png = custom_target(logo + '.png',
                input : logo + '.svg',
                output: logo + '.png',
                command: [
                colorsvg2png, '@INPUT@', '@OUTPUT@', '128',
                ],
                install: true,
                install_dir: gimpdatadir / 'images')

  if logo == docs_logo and gi_docgen.found()
    install_symlink(logo + '.png',
                    install_dir: get_option('datadir') / 'doc' / 'gimp-@0@'.format(gimp_app_version) / 'images',
                    pointing_to: logo_png.full_path())
  endif
endforeach
