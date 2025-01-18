import os
import sys

image     = Gimp.get_images()[0]
procedure = Gimp.get_pdb().lookup_procedure("file-png-export")
config    = procedure.create_config()
image.flatten()
image.scale(1920, 1080)
drawables = image.get_selected_drawables()
config.set_property("image", image)
config.set_property("file", Gio.file_new_for_path("gimp-data/images/gimp-splash.png"))
config.set_property("time", False)
retval = procedure.run(config)
if retval.index(0) != Gimp.PDBStatusType.SUCCESS:
  sys.exit(os.EX_SOFTWARE)
