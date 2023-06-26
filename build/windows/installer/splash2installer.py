image  = Gimp.list_images()[0]
config = Gimp.get_pdb().lookup_procedure("file-bmp-save").create_config()

def export_scaled_img(image, target_width, target_height, export_path):
  img        = image.duplicate()
  w          = img.get_width()
  h          = img.get_height()
  new_width  = target_width
  new_height = target_height
  offx       = 0
  offy       = 0
  if w / target_width * target_height < h:
    new_width = target_height / h * w
    offx = (target_width - new_width) / 2
  else:
    new_height = target_width / w * h
    offy = (target_height - new_height) / 2
  img.scale(new_width, new_height)
  img.resize(target_width, target_height, offx, offy)
  # XXX: should we rather use the average color as border?
  black = Gimp.RGB()
  black.set(0, 0, 0)
  Gimp.context_set_background(black)
  drawables = img.list_selected_drawables()
  for d in drawables:
    d.resize_to_image_size()

  config.set_property("image", img)
  config.set_property("num-drawables", len(drawables))
  config.set_property("drawables", Gimp.ObjectArray.new(Gimp.Drawable, drawables, False))
  config.set_property("file", Gio.file_new_for_path(export_path))
  Gimp.get_pdb().run_procedure_config("file-bmp-save", config)

# These sizes are pretty much hardcoded, and in particular the ratio matters in
# InnoSetup. Or so am I told. XXX
export_scaled_img(image, 994, 692, 'build/windows/installer/installsplash-devel.bmp')
export_scaled_img(image, 497, 360, 'build/windows/installer/installsplash_small-devel.bmp')
export_scaled_img(image, 1160, 803, 'build/windows/installer/installsplash_big-devel.bmp')
