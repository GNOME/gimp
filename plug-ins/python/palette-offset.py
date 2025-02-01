#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#    This program is free software: you can redistribute it and/or modify
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
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.

import gi
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp
gi.require_version('GimpUi', '3.0')
from gi.repository import GimpUi
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio
import sys

def N_(message): return message
def _(message): return GLib.dgettext(None, message)

help_doc = r"""
Offset the colors in the palette.
Offsets and returns the given palette when it is editable,
otherwise copies the given palette and returns it.
"""

class PaletteOffset (Gimp.PlugIn):
    ## GimpPlugIn virtual methods ##
    def do_set_i18n(self, procname):
        return True, 'gimp30-python', None

    def do_query_procedures(self):
        return [ "python-fu-palette-offset" ]

    def do_create_procedure(self, name):
        procedure = Gimp.Procedure.new(self, name,
                                       Gimp.PDBProcType.PLUGIN,
                                       self.run, None)
        if name == 'python-fu-palette-offset':
            procedure.set_menu_label(_("_Offset Palette..."))
            procedure.set_documentation(_("Offset the colors in a palette"),
                                        help_doc,
                                        "")
            procedure.set_attribution("Joao S. O. Bueno Calligaris, Carol Spears",
                                      "(c) Joao S. O. Bueno Calligaris",
                                      "2004, 2006")
            procedure.add_enum_argument ("run-mode", _("Run mode"),
                                         _("The run mode"), Gimp.RunMode,
                                         Gimp.RunMode.NONINTERACTIVE,
                                         GObject.ParamFlags.READWRITE)
            procedure.add_palette_argument ("palette", _("_Palette"),
                                            _("Palette"), True,
                                            None, True, # Default to context.
                                            GObject.ParamFlags.READWRITE)
            procedure.add_int_argument ("amount", _("O_ffset"), _("Offset"),
                                        1, GLib.MAXINT, 1, GObject.ParamFlags.READWRITE)
            procedure.add_palette_return_value ("new-palette", _("The edited palette"),
                                                _("The newly created palette when read-only, otherwise the input palette"),
                                                GObject.ParamFlags.READWRITE)
            procedure.add_menu_path ('<Palettes>/Palettes Menu')
        else:
            procedure = None

        return procedure

    def run(self, procedure, config, data):
        runmode = config.get_property("run-mode")
        palette = config.get_property("palette")
        amount  = config.get_property("amount")

        if palette is None:
            palette = Gimp.context_get_palette()
            config.set_property("palette", palette)

        if not palette.is_valid():
            error = f'Invalid palette ID: {palette.get_id()}'
            return procedure.new_return_values(Gimp.PDBStatusType.CALLING_ERROR,
                                               GLib.Error(error))

        if runmode == Gimp.RunMode.INTERACTIVE:
            gi.require_version('Gtk', '3.0')
            from gi.repository import Gtk

            GimpUi.init ("palette-offset.py")

            dialog = GimpUi.ProcedureDialog(procedure=procedure, config=config)

            config.set_property("palette", None)
            dialog.fill(["palette", "amount"])
            if not dialog.run():
                dialog.destroy()
                return procedure.new_return_values(Gimp.PDBStatusType.CANCEL, GLib.Error())
            else:
                dialog.destroy()

            amount = config.get_property("amount")
            palette = config.get_property("palette")

        #If palette is read only, work on a copy:
        editable = palette.is_editable()
        if not editable:
            palette = palette.duplicate()

        num_colors = palette.get_color_count()
        tmp_entry_array = []
        for i in range (num_colors):
            tmp_entry_array.append  ((palette.get_entry_name(i)[1],
                                      palette.get_entry_color(i)))
        for i in range (num_colors):
            target_index = i + amount
            if target_index >= num_colors:
                target_index -= num_colors
            elif target_index < 0:
                target_index += num_colors
            palette.set_entry_name(target_index, tmp_entry_array[i][0])
            palette.set_entry_color(target_index, tmp_entry_array[i][1])

        retval = procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())
        value = GObject.Value(Gimp.Palette, palette)
        retval.remove(1)
        retval.insert(1, value)
        return retval

Gimp.main(PaletteOffset.__gtype__, sys.argv)
