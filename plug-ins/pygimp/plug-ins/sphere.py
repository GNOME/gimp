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

import math
from gimpfu import *

def sphere(radius, light, shadow, foo, bg_colour, sphere_colour):
    if radius < 1:
        radius = 1

    width = int(radius * 3.75)
    height = int(radius * 2.5)

    gimp.context_push()

    img = gimp.Image(width, height, RGB)

    drawable = gimp.Layer(img, "Sphere Layer", width, height,
                          RGB_IMAGE, 100, NORMAL_MODE)

    radians = light * math.pi / 180

    cx = width / 2
    cy = height / 2

    light_x = cx + radius * 0.6 * math.cos(radians)
    light_y = cy - radius * 0.6 * math.sin(radians)

    light_end_x = cx + radius * math.cos(math.pi + radians)
    light_end_y = cy - radius * math.sin(math.pi + radians)

    offset = radius * 0.1

    img.disable_undo()
    img.insert_layer(drawable)

    gimp.set_foreground(sphere_colour)

    gimp.set_background(bg_colour)
    pdb.gimp_edit_fill(drawable, BACKGROUND_FILL)

    gimp.set_background(20, 20, 20)

    if (light >= 45 and light <= 75 or light <= 135 and
        light >= 105) and shadow:
        shadow_w = radius * 2.5 * math.cos(math.pi + radians)
        shadow_h = radius * 0.5
        shadow_x = cx
        shadow_y = cy + radius * 0.65

        if shadow_w < 0:
            shadow_x = cx + shadow_w
            shadow_w = -shadow_w

        pdb.gimp_ellipse_select(img, shadow_x, shadow_y, shadow_w, shadow_h,
                                CHANNEL_OP_REPLACE, True, True, 7.5)
        pdb.gimp_edit_bucket_fill(drawable, BG_BUCKET_FILL,
                                  MULTIPLY_MODE, 100, 0, False, 0, 0)

    pdb.gimp_ellipse_select(img, cx - radius, cy - radius, 2 * radius,
                            2 * radius, CHANNEL_OP_REPLACE, True, False, 0)
    pdb.gimp_edit_blend(drawable, FG_BG_RGB_MODE, NORMAL_MODE, GRADIENT_RADIAL,
                        100, offset, REPEAT_NONE, False, False, 0, 0, True,
                        light_x, light_y, light_end_x, light_end_y)

    pdb.gimp_selection_none(img)

    img.enable_undo()

    disp = gimp.Display(img)

    gimp.context_pop()


register(
    "python-fu-sphere",
    "Simple sphere with drop shadow",
    "Simple sphere with drop shadow",
    "James Henstridge",
    "James Henstridge",
    "1997-1999, 2007",
    "_Sphere",
    "",
    [
        (PF_INT, "radius", "Radius for sphere", 100),
        (PF_SLIDER, "light", "Light angle", 45, (0,360,1)),
        (PF_TOGGLE, "shadow", "Shadow?", 1),
        (PF_RADIO, "foo", "Test", "foo", (("Foo", "foo"), ("Bar", "bar"))),
        (PF_COLOR, "bg-color", "Background", (1.0, 1.0, 1.0)),
        (PF_COLOR, "sphere-color", "Sphere", "orange")
    ],
    [],
    sphere,
    menu="<Image>/Filters/Languages/Python-Fu/Test")

main()
