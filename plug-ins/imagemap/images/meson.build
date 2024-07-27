icon_type = 'scalable'
icon_format = 'svg'

if not have_vector_icons
  icon_type = 'legacy'
  icon_format = 'png'
endif

stock_images = [
  'imagemap-circle' + icon_format,
  'imagemap-coord' + icon_format,
  'imagemap-dimension' + icon_format,
  'imagemap-java' + icon_format,
  'imagemap-polygon' + icon_format,
  'imagemap-rectangle' + icon_format,
  'imagemap-to-back' + icon_format,
  'imagemap-to-front' + icon_format,
]

imagemap_icon_sources = gnome.compile_resources(
  'imagemap-icon-images',
  'imagemap-icon-' + icon_type + '-images.gresource.xml',
  # source_dir: meson.current_source_directory(),
)
