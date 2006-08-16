#!/usr/bin/env python

#   Gimp-Python - allows the writing of Gimp plugins in Python.
#   Copyright (C) 1997  James Henstridge <james@daa.com.au>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

from gimpfu import *

def shadow_bevel(img, drawable, blur, bevel, do_shadow, drop_x, drop_y):
    # disable undo for the image
    img.undo_group_start()

    # copy the layer
    shadow = drawable.copy(True)
    img.add_layer(shadow, img.layers.index(drawable)+1)
    shadow.name = drawable.name + " shadow"
    shadow.lock_alpha = False

    # threshold the shadow layer to all white
    pdb.gimp_threshold(shadow, 0, 255)

    # blur the shadow layer
    pdb.plug_in_gauss_iir(img, shadow, blur, True, True)

    # do the bevel thing ...
    if bevel:
        pdb.plug_in_bump_map(img, drawable, shadow, 135, 45, 3,
                             0, 0, 0, 0, True, False, 0)

    # make the shadow layer black now ...
    pdb.gimp_invert(shadow)

    # translate the drop shadow
    shadow.translate(drop_x, drop_y)

    if not do_shadow:
        # delete shadow ...
        gimp.delete(shadow)

    # enable undo again
    img.undo_group_end()

def query_shadow_bevel():
    pdb.gimp_plugin_menu_register("python_fu_shadow_bevel",
				  "<Image>/Filters/Light and Shadow/Shadow")
register(
    "python_fu_shadow_bevel",
    "Add a drop shadow to a layer, and optionally bevel it",
    "Add a drop shadow to a layer, and optionally bevel it",
    "James Henstridge",
    "James Henstridge",
    "1999",
    "_Drop Shadow and Bevel...",
    "RGBA, GRAYA",
    [
        (PF_SLIDER, "blur",   "Shadow blur", 6, (1, 30, 1)),
        (PF_BOOL,   "bevel",  "Bevel the image", True),
        (PF_BOOL,   "shadow", "Make a drop shadow", True),
        (PF_INT,    "drop_x", "Drop shadow X displacement", 3),
        (PF_INT,    "drop_y", "Drop shadow Y displacement", 6)
    ],
    [],
    shadow_bevel, on_query=query_shadow_bevel)

main()
