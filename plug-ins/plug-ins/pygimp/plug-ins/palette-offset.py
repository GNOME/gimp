#!/usr/bin/env python
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

gettext.install("gimp20-python", gimp.locale_directory, unicode=True)

def palette_offset(palette, amount):
    #If palette is read only, work on a copy:
    editable = pdb.gimp_palette_is_editable(palette) 
    if not editable:palette = pdb.gimp_palette_duplicate (palette)     

    num_colors = pdb.gimp_palette_get_info (palette)

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


register(
    "python-fu-palette-offset",
    N_("Offset the colors in a palette"),
    "palette_offset (palette, amount) -> modified_palette",
    "Joao S. O. Bueno Calligaris, Carol Spears",
    "(c) Joao S. O. Bueno Calligaris",
    "2004, 2006",
    N_("_Offset Palette..."),
    "",
    [
     (PF_PALETTE, "palette", _("Palette"), ""),
     (PF_INT,     "amount",  _("Off_set"),  1),
    ],
    [(PF_PALETTE, "new-palette", "Result")],
    palette_offset,
    menu="<Palettes>",
    domain=("gimp20-python", gimp.locale_directory)
    )

main ()
