import pathlib
import os
import sys

image     = Gimp.get_images()[0]
procedure = Gimp.get_pdb().lookup_procedure("file-bmp-export")
if procedure is None:
  sys.exit(os.EX_OK)
config    = procedure.create_config()


def export_scaled_img(image, target_height, export_path):
  img    = image.duplicate()
  img.undo_disable()
  layer  = img.flatten()
  width  = layer.get_width()
  height = layer.get_height()
  target_width  = int(target_height / height * width)
  layer.scale(target_width, target_height, False)
  img.resize(target_width, target_height, 0, 0)

  config.set_property("image", img)
  config.set_property("file", Gio.file_new_for_path(export_path))
  retval = procedure.run(config)
  if retval.index(0) != Gimp.PDBStatusType.SUCCESS:
    sys.exit(70)
  img.delete()


# See https://gitlab.gnome.org/GNOME/gimp-data/-/issues/5
def export_blurred_img(image, target_height, export_path):
  img    = image.duplicate()
  img.undo_disable()
  layer  = img.flatten()
  width  = layer.get_width()
  height = layer.get_height()
  target_width  = int(target_height / height * width)
  layer.scale(target_width, target_height, False)
  img.resize(target_width, target_height, 0, 0)

  filter = Gimp.DrawableFilter.new(layer, "gegl:gaussian-blur", "")
  filter_config = filter.get_config()
  filter_config.set_property('std-dev-x', 27.5)
  filter_config.set_property('std-dev-y', 27.5)
  layer.merge_filter(filter)

  filter = Gimp.DrawableFilter.new(layer, "gegl:noise-hsv", "")
  filter_config = filter.get_config()
  filter_config.set_property('hue-distance', 2.0)
  filter_config.set_property('saturation-distance', 0.002)
  filter_config.set_property('value-distance', 0.015)
  layer.merge_filter(filter)

  config.set_property("image", img)
  config.set_property("file", Gio.file_new_for_path(export_path))
  retval = procedure.run(config)
  if retval.index(0) != Gimp.PDBStatusType.SUCCESS:
    sys.exit(70)
  img.delete()


def export_cropped_img(image, target_width, target_height, export_path):
  img        = image.duplicate()
  img.undo_disable()
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
    sys.exit(70)
  img.delete()


image.undo_disable()

# Safe (taller than necessary) heights derived from: https://jrsoftware.org/ishelp/index.php?topic=setup_wizardimagefile
export_scaled_img(image, 314, 'build/windows/installer/installsplash_top.scale-100.bmp')
export_scaled_img(image, 386, 'build/windows/installer/installsplash_top.scale-125.bmp')
export_scaled_img(image, 459, 'build/windows/installer/installsplash_top.scale-150.bmp')
export_scaled_img(image, 556, 'build/windows/installer/installsplash_top.scale-175.bmp')
export_scaled_img(image, 604, 'build/windows/installer/installsplash_top.scale-200.bmp')
export_scaled_img(image, 700, 'build/windows/installer/installsplash_top.scale-225.bmp')
export_scaled_img(image, 797, 'build/windows/installer/installsplash_top.scale-250.bmp')
export_blurred_img(image, 314, 'build/windows/installer/installsplash_bottom.bmp')

# Avoid the images being re-generated at each build.
pathlib.Path('gimp-data/images/stamp-installsplash.bmp').touch()

# Needed by the installer .iss script to maintain the splash aspect ratio: https://gitlab.gnome.org/GNOME/gimp/-/issues/11677 
with open('build/windows/installer/splash-dimensions.h', 'w') as f:
  f.write('#define GIMP_SPLASH_WIDTH  {}\n'.format(image.get_width()))
  f.write('#define GIMP_SPLASH_HEIGHT {}\n'.format(image.get_height()))


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
