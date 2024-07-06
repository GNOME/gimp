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
  img.flatten()
  if w / target_width * target_height < h:
    new_width = target_height / h * w
    offx = (target_width - new_width) / 2
  else:
    new_height = target_width / w * h
    offy = (target_height - new_height) / 2
  img.scale(new_width, new_height)
  img.resize(target_width, target_height, offx, offy)
  # XXX: should we rather use the average color as border?
  black = Gegl.Color.new("black")
  Gimp.context_set_background(black)
  drawables = img.get_selected_drawables()
  for d in drawables:
    d.resize_to_image_size()

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
export_cropped_img(image, 192, 386, 'build/windows/installer/install-end.scale-125.bmp')
export_cropped_img(image, 246, 459, 'build/windows/installer/install-end.scale-150.bmp')
export_cropped_img(image, 273, 556, 'build/windows/installer/install-end.scale-175.bmp')
export_cropped_img(image, 328, 604, 'build/windows/installer/install-end.scale-200.bmp')
export_cropped_img(image, 355, 700, 'build/windows/installer/install-end.scale-225.bmp')
export_cropped_img(image, 410, 797, 'build/windows/installer/install-end.scale-250.bmp')

# Avoid the images being re-generated at each build.
pathlib.Path('gimp-data/images/stamp-install-end.bmp').touch()
