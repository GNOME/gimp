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


register(
    "python-fu-palette-sort",
    N_("Sort the colors in a palette"),
    "palette_merge (palette, model, channel, ascending) -> new_palette",
    "Joao S. O. Bueno Calligaris, Carol Spears",
    "Joao S. O. Bueno Calligaris",
    "2006",
    N_("_Sort Palette..."),
    "",
    [
        (PF_PALETTE, "palette",  _("Palette"), ""),
        (PF_RADIO,   "model",    _("Color _model"), "HSV",
                                    ((_("RGB"), "RGB"),
                                     (_("HSV"), "HSV"))),
        (PF_RADIO,   "channel",  _("Channel to _sort"), 2,
                                    ((_("Red or Hue"),          0),
                                     (_("Green or Saturation"), 1),
                                     (_("Blue or Value"),       2))),
        (PF_BOOL,   "ascending", _("_Ascending"), True)
    ],
    [],
    palette_sort,
    menu="<Palettes>",
    domain=("gimp20-python", gimp.locale_directory)
    )


main ()
