#!/usr/bin/env python

#   Gimp-Python - allows the writing of Gimp plugins in Python.
#   Copyright (C) 1997  James Henstridge <james@daa.com.au>
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

from gimpfu import *
import time

gettext.install("gimp20-python", gimp.locale_directory, unicode=True)

def foggify(img, layer, name, colour, turbulence, opacity):

    gimp.context_push()
    img.undo_group_start()

    if img.base_type is RGB:
        type = RGBA_IMAGE
    else:
        type = GRAYA_IMAGE
    fog = gimp.Layer(img, name,
                     layer.width, layer.height, type, opacity, NORMAL_MODE)
    fog.fill(TRANSPARENT_FILL)
    img.insert_layer(fog)

    gimp.set_background(colour)
    pdb.gimp_edit_fill(fog, BACKGROUND_FILL)

    # create a layer mask for the new layer
    mask = fog.create_mask(0)
    fog.add_mask(mask)

    # add some clouds to the layer
    pdb.plug_in_plasma(img, mask, int(time.time()), turbulence)

    # apply the clouds to the layer
    fog.remove_mask(MASK_APPLY)

    img.undo_group_end()
    gimp.context_pop()

register(
    "python-fu-foggify",
    N_("Add a layer of fog"),
    "Adds a layer of fog to the image.",
    "James Henstridge",
    "James Henstridge",
    "1999,2007",
    N_("_Fog..."),
    "RGB*, GRAY*",
    [
        (PF_IMAGE, "image",       "Input image", None),
        (PF_DRAWABLE, "drawable", "Input drawable", None),
        (PF_STRING, "name",       _("_Layer name"), _("Clouds")),
        (PF_COLOUR, "colour",     _("_Fog color"),  (240, 180, 70)),
        (PF_SLIDER, "turbulence", _("_Turbulence"), 1.0, (0, 10, 0.1)),
        (PF_SLIDER, "opacity",    _("Op_acity"),    100, (0, 100, 1)),
    ],
    [],
    foggify,
    menu="<Image>/Filters/Render/Clouds",
    domain=("gimp20-python", gimp.locale_directory)
    )

main()
