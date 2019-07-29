#!/usr/bin/env python3
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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import gi
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp
import sys

import gettext
_ = gettext.gettext
def N_(message): return message

def make_gradient(palette, num_segments, num_colors):
    gradient = Gimp.gradient_new(palette)

    if (num_segments > 1):
        Gimp.gradient_segment_range_split_uniform(gradient, 0, -1,
                                                  num_segments)

    for color_number in range(0,num_segments):
        if color_number == num_colors - 1:
            color_number_next = 0
        else:
            color_number_next = color_number + 1
        _, color_left = Gimp.palette_entry_get_color(palette, color_number)
        _, color_right = Gimp.palette_entry_get_color(palette, color_number_next)
        Gimp.gradient_segment_set_left_color(gradient,
                                             color_number, color_left,
                                             100.0)
        Gimp.gradient_segment_set_right_color(gradient,
                                              color_number, color_right,
                                              100.0)
    Gimp.context_set_gradient(gradient)
    retval = [Gimp.param_from_status (Gimp.PDBStatusType.SUCCESS),
              Gimp.param_from_string(gradient)]
    return len(retval), retval

def palette_to_gradient_repeating(palette):
    (_, num_colors) = Gimp.palette_get_info(palette)
    num_segments = num_colors
    return make_gradient(palette, num_segments, num_colors)

def palette_to_gradient(palette):
    (_, num_colors) = Gimp.palette_get_info(palette)
    num_segments = num_colors - 1
    return make_gradient(palette, num_segments, num_colors)

def run(name, n_params, params):
    # run_mode = params[0].get_int32()
    if name == 'python-fu-palette-to-gradient-repeating':
        return palette_to_gradient_repeating(Gimp.context_get_palette())
    else:
        return palette_to_gradient(Gimp.context_get_palette())

def query():
    param = Gimp.ParamDef()
    param.type = Gimp.PDBArgType.INT32
    param.name = "run-mode"
    param.description = _("Run mode")

    retval = Gimp.ParamDef()
    retval.type = Gimp.PDBArgType.STRING
    retval.name = "new-gradient"
    retval.description = _("Result")

    Gimp.install_procedure(
        "python-fu-palette-to-gradient-repeating",
        N_("Create a repeating gradient using colors from the palette"),
        "Create a new repeating gradient using colors from the palette.",
        "Carol Spears, reproduced from previous work by Adrian Likins and Jeff Trefftz",
        "Carol Spears",
        "2006",
        N_("Palette to _Repeating Gradient"),
        "",
        Gimp.PDBProcType.PLUGIN,
        [ param ],
        [ retval ])
    Gimp.plugin_menu_register("python-fu-palette-to-gradient-repeating", "<Palettes>")

    Gimp.install_procedure(
        "python-fu-palette-to-gradient",
        N_("Create a gradient using colors from the palette"),
        "Create a new gradient using colors from the palette.",
        "Carol Spears, reproduced from previous work by Adrian Likins and Jeff Trefftz",
        "Carol Spears",
        "2006",
        N_("Palette to _Gradient"),
        "",
        Gimp.PDBProcType.PLUGIN,
        [ param ],
        [ retval ])
    Gimp.plugin_menu_register("python-fu-palette-to-gradient", "<Palettes>")
    Gimp.plugin_domain_register("gimp30-python", Gimp.locale_directory())

info = Gimp.PlugInInfo ()
info.set_callbacks (None, None, query, run)
Gimp.main_legacy (info, sys.argv)
