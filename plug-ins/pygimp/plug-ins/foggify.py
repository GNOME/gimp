#!/usr/bin/env python

from gimpfu import *
import time

def python_foggify(img, layer, name, colour, turbulence, opacity):
    img.undo_group_start()

    fog = gimp.Layer(img, name, layer.width, layer.height, RGBA_IMAGE,
                     opacity, NORMAL_MODE)
    img.add_layer(fog, 0)
    oldbg = gimp.get_background()
    gimp.set_background(colour)
    pdb.gimp_edit_fill(fog, BACKGROUND_FILL)
    gimp.set_background(oldbg)

    # create a layer mask for the new layer
    mask = fog.create_mask(0)
    fog.add_mask(mask)
        
    # add some clouds to the layer
    pdb.plug_in_plasma(img, mask, int(time.time()), turbulence)
        
    # apply the clouds to the layer
    fog.remove_mask(MASK_APPLY)

    img.undo_group_end()

register(
    "python_fu_foggify",
    "Add a layer of fog to the image",
    "Add a layer of fog to the image",
    "James Henstridge",
    "James Henstridge",
    "1999",
    "<Image>/Python-Fu/Effects/Add _fog",
    "RGB*, GRAY*",
    [
        (PF_STRING, "name", "The new layer name", "Clouds"),
        (PF_COLOUR, "colour", "The colour of the fog", (240,180,70)),
        (PF_SLIDER, "turbulence", "The turbulence", 1.0, (0, 10, 0.1)),
        (PF_SLIDER, "opacity", "The opacity", 100, (0, 100, 1)),
    ],
    [],
    python_foggify)

main()
