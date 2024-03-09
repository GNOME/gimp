# Logo used in About dialog and API reference documentation #

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

# Logo used in the desktop (system menus/overview, title bar, etc.) #

desktop_sizes = [
  '16',
  '22',
  '24',
  '32',
  '36',
  '48',
  '64',
  '72',
  '96',
  '128',
  '192',
  '256',
  '512',
]

foreach size : desktop_sizes
  sizedir = size + 'x' + size
  installdir = get_option('datadir') / 'icons' / 'hicolor' / sizedir / 'apps'
  app_icon = custom_target('gimp' + sizedir + '.png',
                           input : 'gimp-logo.svg',
                           output: 'gimp' + sizedir + '.png',
                           command: [
                             colorsvg2png, '@INPUT@', '@OUTPUT@', size,
                           ],
                           build_by_default: true,
                           install: false)
  meson.add_install_script('sh', '-c',
                           'mkdir -p "$MESON_INSTALL_DESTDIR_PREFIX/@1@/" && cp -f "@0@" "$MESON_INSTALL_DESTDIR_PREFIX/@1@/gimp.png"'.format(app_icon.full_path(), installdir))
endforeach

install_data('gimp-logo.svg',
             install_dir: get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
             rename: 'gimp.svg')
