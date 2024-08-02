icon_type = 'scalable'
icon_format = 'svg'

if not have_vector_icons
  icon_type = 'legacy'
  icon_format = 'png'
endif

stock_icons = [
  'gfig-bezier.' + icon_format,
  'gfig-circle.' + icon_format,
  'gfig-copy-object.' + icon_format,
  'gfig-curve.' + icon_format,
  'gfig-delete-object.' + icon_format,
  'gfig-ellipse.' + icon_format,
  'gfig-line.' + icon_format,
  'gfig-logo.png',
  'gfig-move-object.' + icon_format,
  'gfig-move-point.' + icon_format,
  'gfig-polygon.' + icon_format,
  'gfig-rectangle.' + icon_format,
  'gfig-show-all.' + icon_format,
  'gfig-spiral.' + icon_format,
  'gfig-star.' + icon_format,
]

gfig_icon_sources = gnome.compile_resources(
  'gfig-icon-images',
  'gfig-icon-' + icon_type + '-images.gresource.xml',
  # source_dir: meson.current_source_directory(),
)
