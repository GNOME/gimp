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
    return gradient

def run(procedure, args, data):
    # Get the parameters
    palette = None
    if args.length() > 1:
        palette = args.index(1)
    if palette == '' or palette is None:
        palette = Gimp.context_get_palette()
    (exists, num_colors) = Gimp.palette_get_info(palette)
    if not exists:
        error = 'Unknown palette: {}'.format(palette)
        return procedure.new_return_values(Gimp.PDBStatusType.CALLING_ERROR,
                                           GLib.Error(error))

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

    # XXX: I don't try to get the GValue with retval.index(1) because it
    # actually return a string (cf. pygobject#353). Just create a new
    # GValue and replace the default one with this one.
    value = GObject.Value(GObject.TYPE_STRING, gradient)
    retval.remove(1)
    retval.insert(1, value)

    return retval

class PaletteToGradient (Gimp.PlugIn):
    ## Parameter: run mode ##
    @GObject.Property(type=Gimp.RunMode,
                      default=Gimp.RunMode.NONINTERACTIVE,
                      nick="Run mode", blurb="The run mode")
    def run_mode(self):
        '''The run mode (unused)'''
        return self.runmode

    @run_mode.setter
    def run_mode(self, runmode):
        self.runmode = runmode

    ## Parameter: palette ##
    @GObject.Property(type=str,
                      default=None,
                      nick= _("Palette"))
    def palette(self):
        '''Palette name or empty string for the currently selected palette'''
        return self.palette

    @palette.setter
    def palette(self, palette):
        self.palette = palette

    ## Properties: return values ##
    @GObject.Property(type=str,
                      default="",
                      nick="Name of the newly created gradient",
                      blurb="Name of the newly created gradient")
    def new_gradient(self):
        """Read-write integer property."""
        return self.new_gradient

    @new_gradient.setter
    def new_gradient(self, new_gradient):
        self.new_gradient = new_gradient

    ## GimpPlugIn virtual methods ##
    def do_query_procedures(self):
        # Localization
        self.set_translation_domain ("gimp30-python",
                                     Gio.file_new_for_path(Gimp.locale_directory()))

        return ['python-fu-palette-to-gradient',
                'python-fu-palette-to-gradient-repeating']

    def do_create_procedure(self, name):
        procedure = Gimp.Procedure.new(self, name,
                                       Gimp.PDBProcType.PLUGIN,
                                       run, None)
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
            # We don't build a GParamSpec ourselves because passing it
            # around is apparently broken in Python. Hence this trick.
            # See pygobject#227
            procedure.add_argument_from_property(self, "run-mode")
            procedure.add_argument_from_property(self, "palette")
            procedure.add_return_value_from_property(self, "new-gradient")

            procedure.add_menu_path ('<Palettes>')

        return procedure

Gimp.main(PaletteToGradient.__gtype__, sys.argv)
