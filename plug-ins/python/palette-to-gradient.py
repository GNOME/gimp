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
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio
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

def palette_to_gradient(procedure, args, data):
    # Localization
    plug_in = procedure.get_plug_in()
    plug_in.set_translation_domain ("gimp30-python",
                                    Gio.file_new_for_path(Gimp.locale_directory()))

    palette = Gimp.context_get_palette()
    (_, num_colors) = Gimp.palette_get_info(palette)

    if procedure.get_name() == 'python-fu-palette-to-gradient':
        num_segments = num_colors - 1
    else: # 'python-fu-palette-to-gradient-repeating'
        num_segments = num_colors
    gradient = make_gradient(palette, num_segments, num_colors)

    # XXX: for the error parameter, we want to return None.
    # Unfortunately even though the argument is (nullable), pygobject
    # looks like it may have a bug. So workaround is to just set a
    # generic GLib.Error() since anyway the error won't be process with
    # GIMP_PDB_SUCCESS status.
    # See pygobject#351
    retval = procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())

    # TODO: uncomment when GParamSpec bug is fixed.
    #value = retval.index(1)
    #value.set_string (gradient);
    return retval

class PaletteToGradient (Gimp.PlugIn):
    def do_query_procedures(self):
        # XXX See pygobject#352 for the weird return value.
        return ['python-fu-palette-to-gradient',
                'python-fu-palette-to-gradient-repeating'], 2

    def do_create_procedure(self, name):
        procedure = Gimp.Procedure.new(self, name,
                                       Gimp.PDBProcType.PLUGIN,
                                       palette_to_gradient, None)
        if name == 'python-fu-palette-to-gradient':
            procedure.set_menu_label(N_("Palette to _Gradient"))
            procedure.set_documentation(N_("Create a gradient using colors from the palette"),
                                        "Create a new gradient using colors from the palette.",
                                        "")
        elif name == 'python-fu-palette-to-gradient-repeating':
            procedure.set_menu_label(N_("Palette to _Repeating Gradient"))
            procedure.set_documentation(N_("Create a repeating gradient using colors from the palette"),
                                        "Create a new repeating gradient using colors from the palette.",
                                        "")
        else:
            procedure = None

        if procedure is not None:
            procedure.set_attribution("Carol Spears, reproduced from previous work by Adrian Likins and Jeff Trefftz",
                                      "Carol Spears", "2006")
            returnspec = GObject.param_spec_string("new-gradient",
                                                    "Name of the newly created gradient",
                                                    "Name of the newly created gradient",
                                                    None,
                                                    GObject.ParamFlags.READWRITE)
            # Passing a GObjectParamSpec currently fails.
            # TODO: see pygobject#227
            #procedure.add_return_value(returnspec)

            paramspec = GObject.param_spec_enum("run-mode",
                                                "Run mode",
                                                "The run mode",
                                                Gimp.RunMode.__gtype__,
                                                Gimp.RunMode.NONINTERACTIVE,
                                                GObject.ParamFlags.READWRITE)
            # TODO: see pygobject#227
            #procedure.add_argument(paramspec)

            # TODO: To be installed in '<Palettes>', we need a run mode
            # parameter. Wait for the GParamSpec bug to be fixed before
            # uncommenting.
            #procedure.add_menu_path ('<Palettes>')

        return procedure

Gimp.main(PaletteToGradient.__gtype__, sys.argv)
