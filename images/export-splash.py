from gi.repository import Pango
import os
import sys

image = Gimp.get_images()[0]

def find_rc_layer(layers):
  for layer in layers:
    if type(layer) is Gimp.TextLayer:
      text = layer.get_text()
      if text is not None and text.strip() == 'RC':
        return layer
      else:
        markup = layer.get_markup()
        _, _, text, _ = Pango.parse_markup(markup, -1, '_')
        if text.strip() == 'RC':
          return layer
    elif type(layer) is Gimp.GroupLayer:
      child = find_rc_layer(layer.get_children())
      if child is not None:
        return child
  return None

if Gimp.MINOR_VERSION % 2 == 0 and Gimp.MICRO_VERSION % 2 == 0:
  rc_layer = find_rc_layer(image.get_layers())

  if rc_layer is None:
    msg = 'ERROR: a text layer containing the text "RC" is mandatory for RC and stable splash images.'
    sys.stderr.write('v' * len(msg) + '\n')
    sys.stderr.write(f'{msg}\n')
    sys.stderr.write('^' * len(msg) + '\n')
    sys.exit(70)

  if 'RC' not in Gimp.VERSION:
    # This is a stable release (and not a release candidate).
    # Drop the mandatory "RC" layer.
    image.remove_layer(rc_layer)

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
