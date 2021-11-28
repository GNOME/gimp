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
