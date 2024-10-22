import pathlib
import os
import sys

base_path = '../gimp-data/images/logo/fileicon-base.svg'
wilber_path = '../gimp-data/images/logo/gimp-logo-shadow.svg'
export_dir = 'build/windows/store/Assets/'


def export_relative_img(scale):
  # 1. Loading the "File" SVG base.
  procedure   = Gimp.get_pdb().lookup_procedure("file-svg-load")
  config      = procedure.create_config()
  config.set_property("file", Gio.file_new_for_path(base_path))
  size        = 44 * scale / 100
  config.set_property("width",  size)
  config.set_property("height", size)
  Gimp.Procedure.run(procedure, config)
  v = Gimp.Procedure.run(procedure, config)

  if v.index(0) != Gimp.PDBStatusType.SUCCESS:
    sys.exit(os.EX_SOFTWARE)

  image = v.index(1)

  # 2. Loading Wilber.
  config.set_property("file", Gio.file_new_for_path(wilber_path))
  wilber_size = size * 7 / 8
  config.set_property("width",  wilber_size)
  config.set_property("height", wilber_size)
  Gimp.Procedure.run(procedure, config)
  v = Gimp.Procedure.run(procedure, config)

  if v.index(0) != Gimp.PDBStatusType.SUCCESS:
    sys.exit(os.EX_SOFTWARE)

  tmp_image = v.index(1)
  drawables = tmp_image.get_selected_drawables()
  layer2 = Gimp.Layer.new_from_drawable (drawables[0], image)
  image.insert_layer(layer2, None, 0)
  layer2.set_offsets(0, (size - wilber_size) * 5 / 6)
  tmp_image.delete()

  # 3. Merging the file design and Wilber on top.
  image.merge_down(layer2, Gimp.MergeType.CLIP_TO_IMAGE)

  if scale == 100:
    procedure = Gimp.get_pdb().lookup_procedure("file-png-export")
    config    = procedure.create_config()
    drawables = image.get_selected_drawables()
    config.set_property("image", image)
    config.set_property("file", Gio.file_new_for_path(export_dir + 'fileicon.png'))
    Gimp.Procedure.run(procedure, config)

  procedure = Gimp.get_pdb().lookup_procedure("file-png-export")
  config    = procedure.create_config()
  drawables = image.get_selected_drawables()
  config.set_property("image", image)
  scale_str = str(scale)
  config.set_property("file", Gio.file_new_for_path(export_dir + 'fileicon.scale-' + scale_str + '.png'))
  Gimp.Procedure.run(procedure, config)


def export_absolute_img(size):
  # 1. Loading the "File" SVG base.
  procedure   = Gimp.get_pdb().lookup_procedure("file-svg-load")
  config      = procedure.create_config()
  config.set_property("file", Gio.file_new_for_path(base_path))
  config.set_property("width",  size)
  config.set_property("height", size)
  Gimp.Procedure.run(procedure, config)
  v = Gimp.Procedure.run(procedure, config)

  if v.index(0) != Gimp.PDBStatusType.SUCCESS:
    sys.exit(os.EX_SOFTWARE)

  image = v.index(1)

  # 2. Loading Wilber.
  config.set_property("file", Gio.file_new_for_path(wilber_path))
  wilber_size = size * 7 / 8
  config.set_property("width",  wilber_size)
  config.set_property("height", wilber_size)
  Gimp.Procedure.run(procedure, config)
  v = Gimp.Procedure.run(procedure, config)

  if v.index(0) != Gimp.PDBStatusType.SUCCESS:
    sys.exit(os.EX_SOFTWARE)

  tmp_image = v.index(1)
  drawables = tmp_image.get_selected_drawables()
  layer2 = Gimp.Layer.new_from_drawable (drawables[0], image)
  image.insert_layer(layer2, None, 0)
  layer2.set_offsets(0, (size - wilber_size) * 5 / 6)
  tmp_image.delete()

  # 3. Merging the file design and Wilber on top.
  image.merge_down(layer2, Gimp.MergeType.CLIP_TO_IMAGE)

  if size == 44:
    procedure = Gimp.get_pdb().lookup_procedure("file-png-export")
    config    = procedure.create_config()
    drawables = image.get_selected_drawables()
    config.set_property("image", image)
    config.set_property("file", Gio.file_new_for_path(export_dir + 'fileicon.png'))
    Gimp.Procedure.run(procedure, config)

  else:
    procedure = Gimp.get_pdb().lookup_procedure("file-png-export")
    config    = procedure.create_config()
    drawables = image.get_selected_drawables()
    config.set_property("image", image)
    size_str = str(size)
    config.set_property("file", Gio.file_new_for_path(export_dir + 'fileicon.targetsize-' + size_str + '.png'))
    Gimp.Procedure.run(procedure, config)


export_relative_img(100)
export_relative_img(125)
export_relative_img(150)
export_relative_img(200)
export_relative_img(400)

export_absolute_img(44)
export_absolute_img(16)
export_absolute_img(20)
export_absolute_img(24)
export_absolute_img(30)
export_absolute_img(32)
export_absolute_img(36)
export_absolute_img(40)
export_absolute_img(48)
export_absolute_img(56)
export_absolute_img(60)
export_absolute_img(64)
export_absolute_img(72)
export_absolute_img(80)
export_absolute_img(96)
export_absolute_img(128)
export_absolute_img(256)


# Avoid the images being re-generated at each build.
pathlib.Path('gimp-data/images/logo/stamp-fileicon.png').touch()
