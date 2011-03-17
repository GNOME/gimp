#!/usr/bin/env python
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from gimpfu import *

gettext.install("gimp20-python", gimp.locale_directory, unicode=True)

def make_gradient(palette, num_segments, num_colors):
    gradient = pdb.gimp_gradient_new(palette)

    if (num_segments > 1):
        pdb.gimp_gradient_segment_range_split_uniform(gradient, 0, -1,
                                                      num_segments)

    for color_number in range(0,num_segments):
        if (color_number == num_colors-1):color_number_next = 0
        else: color_number_next = color_number + 1
        color_left = pdb.gimp_palette_entry_get_color(palette,
                                                      color_number)
        color_right = pdb.gimp_palette_entry_get_color(palette,
                                                       color_number_next)
        pdb.gimp_gradient_segment_set_left_color(gradient,
                                                 color_number, color_left,
                                                 100.0)
        pdb.gimp_gradient_segment_set_right_color(gradient,
                                                  color_number, color_right,
                                                  100.0)
    pdb.gimp_context_set_gradient(gradient)
    return gradient


def palette_to_gradient_repeating(palette):
    num_colors = pdb.gimp_palette_get_info(palette)
    num_segments = num_colors
    return make_gradient(palette, num_segments, num_colors)


register(
    "python-fu-palette-to-gradient-repeating",
    N_("Create a repeating gradient using colors from the palette"),
    "Create a new repeating gradient using colors from the palette.",
    "Carol Spears, reproduced from previous work by Adrian Likins and Jeff Trefftz",
    "Carol Spears",
    "2006",
    N_("Palette to _Repeating Gradient"),
    "",
    [(PF_PALETTE,  "palette", _("Palette"), "")],
    [(PF_GRADIENT, "new-gradient", "Result")],
    palette_to_gradient_repeating,
    menu="<Palettes>",
    domain=("gimp20-python", gimp.locale_directory)
    )


def palette_to_gradient(palette):
    num_colors = pdb.gimp_palette_get_info(palette)
    num_segments = num_colors - 1
    return make_gradient(palette, num_segments, num_colors)

register(
    "python-fu-palette-to-gradient",
    N_("Create a gradient using colors from the palette"),
    "Create a new gradient using colors from the palette.",
    "Carol Spears, reproduced from previous work by Adrian Likins and Jeff Trefftz",
    "Carol Spears",
    "2006",
    N_("Palette to _Gradient"),
    "",
    [(PF_PALETTE,  "palette", _("Palette"), "")],
    [(PF_GRADIENT, "new-gradient", "Result")],
    palette_to_gradient,
    menu="<Palettes>",
    domain=("gimp20-python", gimp.locale_directory)
    )

main ()
