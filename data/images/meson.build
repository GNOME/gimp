ligma_logo_dir = meson.current_source_dir()

images = [
  'ligma-splash.png',
  'wilber.png',
]
images += stable ? 'ligma-logo.png' : 'ligma-devel-logo.png'

install_data(
  images,
  install_dir: ligmadatadir / 'images',
)

if gi_docgen.found()
  install_data(
    stable ? 'ligma-logo.png' : 'ligma-devel-logo.png',
    install_dir: get_option('datadir') / 'doc' / 'ligma-@0@'.format(ligma_app_version) / 'images',
  )
endif
