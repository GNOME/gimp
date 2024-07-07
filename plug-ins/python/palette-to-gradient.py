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
gi.require_version('GimpUi', '3.0')
from gi.repository import GimpUi
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
import sys

def N_(message): return message
def _(message): return GLib.dgettext(None, message)


def make_gradient(palette, num_segments, num_colors):

    # name the gradient same as the source palette
    # For now, the name of a resource is the same as the ID
    palette_name = palette.get_name()
    gradient = Gimp.Gradient.new(palette_name)
    # assert gradient is valid but is has only one segment
    assert gradient.get_number_of_segments() == 1

    # split one segment into desired count
    # index is zero-based
    gradient.segment_range_split_uniform( 0, 0, num_segments)

    for color_number in range(0,num_segments):
        if color_number == num_colors - 1:
            color_number_next = 0
        else:
            color_number_next = color_number + 1
        color_left = palette.entry_get_color(color_number)
        color_right = palette.entry_get_color(color_number_next)
        gradient.segment_set_left_color(color_number, color_left)
        gradient.segment_set_right_color(color_number, color_right)

    # Side effects on the context. Probably not what most would expect.
    Gimp.context_set_gradient(gradient)
    return gradient

def run(procedure, config, data):
    # Get the parameters
    run_mode = config.get_property("run-mode")

    if run_mode == Gimp.RunMode.INTERACTIVE:
        GimpUi.init(procedure.get_name())
        dialog = GimpUi.ProcedureDialog(procedure=procedure, config=config)

        # Add palette button
        dialog.fill (["palette"])

        if not dialog.run():
            dialog.destroy()
            return procedure.new_return_values(Gimp.PDBStatusType.CANCEL, GLib.Error())

        dialog.destroy()

    palette = config.get_property("palette")
    if palette is None or not palette.is_valid():
        if palette is not None:
          sys.stderr.write(f'Invalid palette id: {palette.get_id()}\n')
          sys.stderr.write('This should not happen. Please report to GIMP project.\n')
          sys.stderr.write('Falling back to context palette instead.\n')
        palette = Gimp.context_get_palette()
        config.set_property("palette", palette)

    num_colors = palette.get_color_count()

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
    # This comment dates from when resources are strings:
    # value = GObject.Value(GObject.TYPE_STRING, gradient)

    value = GObject.Value(Gimp.Gradient, gradient)
    retval.remove(1)
    retval.insert(1, value)

    return retval

class PaletteToGradient (Gimp.PlugIn):
    ## GimpPlugIn virtual methods ##
    def do_set_i18n(self, procname):
        return True, 'gimp30-python', None

    def do_query_procedures(self):
        return ['python-fu-palette-to-gradient',
                'python-fu-palette-to-gradient-repeating']

    def do_create_procedure(self, name):
        procedure = Gimp.Procedure.new(self, name,
                                       Gimp.PDBProcType.PLUGIN,
                                       run, None)
        if name == 'python-fu-palette-to-gradient':
            procedure.set_menu_label(_("Palette to _Gradient"))
            procedure.set_documentation(_("Create a gradient using colors from the palette"),
                                        _("Create a new gradient using colors from the palette."),
                                        "")
        elif name == 'python-fu-palette-to-gradient-repeating':
            procedure.set_menu_label(_("Palette to _Repeating Gradient"))
            procedure.set_documentation(_("Create a repeating gradient using colors from the palette"),
                                        _("Create a new repeating gradient using colors from the palette."),
                                        "")
        else:
            procedure = None

        if procedure is not None:
            procedure.set_attribution("Carol Spears, reproduced from previous work by Adrian Likins and Jeff Trefftz",
                                      "Carol Spears", "2006")

            procedure.add_enum_argument ("run-mode", _("Run mode"),
                                         _("The run mode"), Gimp.RunMode,
                                         Gimp.RunMode.NONINTERACTIVE,
                                         GObject.ParamFlags.READWRITE)
            procedure.add_palette_argument ("palette", _("_Palette"),
                                            _("Palette"), True,
                                            GObject.ParamFlags.READWRITE)
            procedure.add_gradient_return_value ("new-gradient", _("The newly created gradient"),
                                                 _("The newly created gradient"),
                                                 GObject.ParamFlags.READWRITE)

            procedure.add_menu_path ('<Palettes>/Palettes Menu')

        return procedure

Gimp.main(PaletteToGradient.__gtype__, sys.argv)
