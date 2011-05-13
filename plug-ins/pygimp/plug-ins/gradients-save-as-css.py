#! /usr/bin/env python
# -*- coding: utf-8 -*-

#   Allows saving (TODO: and loading) CSS gradient files
#   Copyright (C) 2011 João S. O. Bueno <gwidion@gmail.com>
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


# Currently this exports all color segments as RGB linear centered segments.
# TODO: Respect gradient alpha, off-center segments, different blending
# functions and HSV colors 
    
from gimpfu import *

gettext.install("gimp20-python", gimp.locale_directory, unicode=True)

w3c_template = """background-image: linear-gradient(top, %s);\n"""
moz_template = """background-image: -moz-linear-gradient(center top, %s);\n"""
webkit_template = """background-image: -webkit-gradient(linear, """ \
                  """left top, left bottom, %s);\n"""

color_to_html = lambda c: "rgb(%d,%d,%d)" % tuple(c)[:3]

def format_text(text):
    counter = 0
    new_text = []
    for token in text.split(","):
        if counter + len(token) > 77:
            token = "\n    " + token
            counter = 4
        new_text.append(token)
        if "\n" in token:
            counter = len(token.rsplit("\n")[-1]) + 1
        else:
            counter += len(token) + 1
        
    return ",".join(new_text)
    
def gradient_css_save(gradient, file_name):
    stops = []
    wk_stops = []
    n_segments = pdb.gimp_gradient_get_number_of_segments(gradient)
    last_stop = None
    for index in xrange(n_segments):
        lcolor, lopacity = pdb.gimp_gradient_segment_get_left_color(
                                gradient,
                                index)
        rcolor, ropacity = pdb.gimp_gradient_segment_get_right_color(
                                gradient,
                                index)
        lpos = pdb.gimp_gradient_segment_get_left_pos(gradient, index)
        rpos = pdb.gimp_gradient_segment_get_right_pos(gradient, index)
        
        lstop = color_to_html(lcolor) + " %d%%" % int(100 * lpos)
        wk_lstop = "color-stop(%.03f, %s)" %(lpos, color_to_html(lcolor))
        if lstop != last_stop:
            stops.append(lstop)
            wk_stops.append(wk_lstop)
        
        rstop = color_to_html(rcolor) + " %d%%" % int(100 * rpos)
        wk_rstop = "color-stop(%.03f, %s)" %(rpos, color_to_html(rcolor))
        
        stops.append(rstop)
        wk_stops.append(wk_rstop)
        last_stop = rstop
    
    final_text = w3c_template % ", ".join(stops)
    final_text += moz_template % ",".join(stops)
    final_text += webkit_template % ",".join(wk_stops)
    
    with open(file_name, "wt") as file_:
        file_.write(format_text(final_text))

register(
        "gradient-save-as-css",
        "Creates a new palette from a given gradient",
        "palette_from_gradient (gradient, number, segment_colors) -> None",
        "Joao S. O. Bueno",
        "(c) GPL V3.0 or later",
        "2011",
        "Save as CSS...",
        "",
        [
          (PF_GRADIENT, "gradient", N_("Gradient to use"),""),
          (PF_FILE, "file_name", N_("File Name"), ""),
        ],
        [],
        gradient_css_save,
        menu="<Gradients>",
        domain=("gimp20-python", gimp.locale_directory)
        )
main()