import os
import sys

image     = Gimp.get_images()[0]
procedure = Gimp.get_pdb().lookup_procedure("file-png-export")
config    = procedure.create_config()
image.undo_disable()
image.flatten()

if image.get_width() > 1920 or image.get_height() > 1080:
  factor = max(image.get_width() / 1920, image.get_height() / 1080)
  new_width  = image.get_width() / factor
  new_height = image.get_height() / factor
  image.scale(new_width, new_height)

drawables = image.get_selected_drawables()
config.set_property("image", image)
config.set_property("file", Gio.file_new_for_path("gimp-data/images/gimp-splash.png"))
config.set_property("time", False)
config.set_property("format", "rgb8")
retval = procedure.run(config)
if retval.index(0) != Gimp.PDBStatusType.SUCCESS:
  sys.exit(70)
