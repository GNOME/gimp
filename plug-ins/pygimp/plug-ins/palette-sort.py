#!/usr/bin/env python
# -*- coding: UTF-8 -*-
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.


from gimpfu import *


def palette_sort (palette, model, channel, ascending):
    #If palette is read only, work on a copy:        
    editable = pdb.gimp_palette_is_editable(palette) 
    if not editable:palette = pdb.gimp_palette_duplicate (palette)     

    num_colors = pdb.gimp_palette_get_info (palette) 
    entry_list = []
    for i in xrange (num_colors):
        entry =  (pdb.gimp_palette_entry_get_name (palette, i),
                  pdb.gimp_palette_entry_get_color (palette, i))
        index = entry[1][channel]
        if model == "HSV":
            index = entry[1].to_hsv()[channel]
        else:
            index = entry[1][channel]
        entry_list.append ((index, entry))
    entry_list.sort(lambda x,y: cmp(x[0], y[0]))
    if not ascending:
        entry_list.reverse()
    for i in xrange(num_colors):
        pdb.gimp_palette_entry_set_name (palette, i, entry_list[i][1][0])
        pdb.gimp_palette_entry_set_color (palette, i, entry_list[i][1][1])  
    
    return palette


def query_sort():
    pdb.gimp_plugin_menu_register("python-fu-palette-sort", "<Palettes>")

register(
    "python-fu-palette-sort",
    "Sort the selected palette.",
    "palette_merge (palette, model, channel, ascending) -> new_palette",
    "Joao S. O. Bueno Calligaris, Carol Spears",
    "Joao S. O. Bueno Calligaris",
    "2006",
    "_Sort Palette...",
    "",
    [
        (PF_PALETTE, "palette", "name of palette to sort", ""),
        (PF_RADIO,   "model", "Color model to sort in  ", "HSV", 
                     (("RGB", "RGB"), 
                      ("HSV", "HSV"))),
        (PF_RADIO,   "channel", "Channel to sort", 2, 
                     (("Red or Hue", 0), 
                      ("Green or Saturation", 1), 
                      ("Blue or Value", 2))),
        (PF_BOOL,   "ascending", "Sort in ascending order", True)
    ],     
    [],
    palette_sort,
    on_query=query_sort)


main ()
