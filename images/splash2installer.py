import pathlib
import os
import sys

image     = Gimp.get_images()[0]
procedure = Gimp.get_pdb().lookup_procedure("file-bmp-export")
if procedure is None:
  sys.exit(os.EX_OK)
config    = procedure.create_config()

def export_scaled_img(image, target_width, target_height, export_path):
  img        = image.duplicate()
  w          = img.get_width()
  h          = img.get_height()
  new_width  = target_width
  new_height = target_height
  offx       = 0
  offy       = 0

  bg_new_width  = target_width
  bg_new_height = target_height
  bg_offx       = 0
  bg_offy       = 0
  layer = img.flatten()
  bg_layer = layer.copy()
  img.insert_layer(bg_layer, None, 1)
  if w / target_width * target_height < h:
    new_width = target_height / h * w
    offx = (target_width - new_width) / 2

    bg_new_height = target_width / w * h
    bg_offy = (target_height - bg_new_height) / 2
  else:
    new_height = target_width / w * h
    offy = (target_height - new_height) / 2

    bg_new_width = target_height / h * w
    bg_offx = (target_width - bg_new_width) / 2
  layer.scale(new_width, new_height, False)
  bg_layer.scale(bg_new_width, bg_new_height, False)
  img.resize(target_width, target_height, offx, offy)
  bg_layer.set_offsets(bg_offx, bg_offy)

  # See https://gitlab.gnome.org/GNOME/gimp-data/-/issues/5
  filter = Gimp.DrawableFilter.new(bg_layer, "gegl:gaussian-blur", "")
  filter_config = filter.get_config()
  filter_config.set_property('std-dev-x', 27.5)
  filter_config.set_property('std-dev-y', 27.5)
  bg_layer.merge_filter(filter)

  filter = Gimp.DrawableFilter.new(bg_layer, "gegl:noise-hsv", "")
  filter_config = filter.get_config()
  filter_config.set_property('hue-distance', 2.0)
  filter_config.set_property('saturation-distance', 0.002)
  filter_config.set_property('value-distance', 0.015)
  bg_layer.merge_filter(filter)

  config.set_property("image", img)
  config.set_property("file", Gio.file_new_for_path(export_path))
  retval = procedure.run(config)
  if retval.index(0) != Gimp.PDBStatusType.SUCCESS:
    sys.exit(os.EX_SOFTWARE)
  img.delete()


def export_cropped_img(image, target_width, target_height, export_path):
  img        = image.duplicate()
  w          = img.get_width()
  h          = img.get_height()
  img.flatten()
  new_width = target_height / h * w
  img.scale(new_width, target_height)
  img.resize(new_width, target_height, 0, 0)
  drawables = img.get_selected_drawables()
  for d in drawables:
    d.resize_to_image_size()
  offx      = new_width * 0.6
  img.crop(target_width, target_height, offx, 0)

  config.set_property("image", img)
  config.set_property("file", Gio.file_new_for_path(export_path))
  retval = procedure.run(config)
  if retval.index(0) != Gimp.PDBStatusType.SUCCESS:
    sys.exit(os.EX_SOFTWARE)
  img.delete()


# These sizes are pretty much hardcoded, and in particular the ratio matters in
# InnoSetup. Or so am I told. XXX
export_scaled_img(image, 994, 692, 'build/windows/installer/installsplash.bmp')
export_scaled_img(image, 497, 360, 'build/windows/installer/installsplash_small.bmp')
export_scaled_img(image, 1160, 803, 'build/windows/installer/installsplash_big.bmp')

# Avoid the images being re-generated at each build.
pathlib.Path('gimp-data/images/stamp-installsplash.bmp').touch()


# https://jrsoftware.org/ishelp/index.php?topic=setup_wizardimagefile
export_cropped_img(image, 164, 314, 'build/windows/installer/install-end.scale-100.bmp')
export_cropped_img(image, 202, 386, 'build/windows/installer/install-end.scale-125.bmp')
export_cropped_img(image, 240, 459, 'build/windows/installer/install-end.scale-150.bmp')
export_cropped_img(image, 290, 556, 'build/windows/installer/install-end.scale-175.bmp')
export_cropped_img(image, 315, 604, 'build/windows/installer/install-end.scale-200.bmp')
export_cropped_img(image, 366, 700, 'build/windows/installer/install-end.scale-225.bmp')
export_cropped_img(image, 416, 797, 'build/windows/installer/install-end.scale-250.bmp')

# Avoid the images being re-generated at each build.
pathlib.Path('gimp-data/images/stamp-install-end.bmp').touch()
