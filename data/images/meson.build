gimp_logo_dir = meson.current_source_dir()

images = [
  'gimp-splash.png',
  'wilber.png',
]
images += stable ? 'gimp-logo.png' : 'gimp-devel-logo.png'

install_data(
  images,
  install_dir: gimpdatadir / 'images',
)

if gi_docgen.found()
  install_data(
    stable ? 'gimp-logo.png' : 'gimp-devel-logo.png',
    install_dir: get_option('datadir') / 'doc' / 'gimp-@0@'.format(gimp_app_version) / 'images',
  )
endif
