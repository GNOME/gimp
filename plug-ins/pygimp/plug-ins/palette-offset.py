#!/usr/bin/python
# -*- coding: utf-8 -*-
#    This program is free software; you can redistribute it and/or modify
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


def palette_offset(palette, amount, forward):
    #If palette is read only, work on a copy:
    editable = pdb.gimp_palette_is_editable(palette) 
    if not editable:palette = pdb.gimp_palette_duplicate (palette)     

    num_colors = pdb.gimp_palette_get_info (palette)
    if not forward:
        amount = num_colors - amount

    tmp_entry_array = []
    for i in xrange (num_colors):
        tmp_entry_array.append  ((pdb.gimp_palette_entry_get_name (palette, i),
                                  pdb.gimp_palette_entry_get_color (palette, i)))
    for i in xrange (num_colors):
        target_index = i + amount
        if target_index >= num_colors:
            target_index -= num_colors
        elif target_index < 0:
            target_index += num_colors
        pdb.gimp_palette_entry_set_name (palette, target_index, tmp_entry_array[i][0])
        pdb.gimp_palette_entry_set_color (palette, target_index, tmp_entry_array[i][1])
    return palette


def query_palette_offset():
    pdb.gimp_plugin_menu_register("python-fu-palette-offset", "<Palettes>")

register(
    "python-fu-palette-offset",
    "Offsets a given palette",
    "palette_offset (palette, amount_to_offset) -> modified_palette",
    "Joao S. O. Bueno Calligaris, Carol Spears",
    "(c) Joao S. O. Bueno Calligaris",
    "2004, 2006",
    "_Offset Palette...",
    "",
    [
     (PF_PALETTE, "palette", "Name of palette to offset", ""),
     (PF_INT,     "amount",  "Amount of colors to offset", ""),
     (PF_BOOL,    "forward", "Offset the palette forward?", True)
    ],
    [(PF_PALETTE, "new-palette", "Name of offset palette.")],
    palette_offset,
    on_query=query_palette_offset)


main ()
