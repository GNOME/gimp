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
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio
import sys

import gettext
_ = gettext.gettext
def N_(message): return message

class PaletteOffset (Gimp.PlugIn):
    ## Parameter: run-mode ##
    @GObject.Property(type=Gimp.RunMode,
                      default=Gimp.RunMode.NONINTERACTIVE,
                      nick="Run mode", blurb="The run mode")
    def run_mode(self):
        """Read-write integer property."""
        return self._run_mode

    @run_mode.setter
    def run_mode(self, run_mode):
        self._run_mode = run_mode

    ## Parameter: palette ##
    @GObject.Property(type=str,
                      default="",
                      nick= _("Palette"),
                      blurb= _("Palette"))
    def palette(self):
        return self._palette

    @palette.setter
    def palette(self, palette):
        self._palette = palette

    ## Parameter: amount ##
    @GObject.Property(type=int,
                      default=1,
                      nick= _("Off_set"),
                      blurb= _("Offset"))
    def amount(self):
        return self._amount

    @amount.setter
    def amount(self, amount):
        self._amount = amount

    ## Return: new-palette ##
    @GObject.Property(type=str,
                      default=None,
                      nick="Name of the edited palette",
                      blurb="Name of the newly created palette if read-only or the input palette otherwise")
    def new_palette(self):
        return self.new_palette

    @new_palette.setter
    def new_palette(self, new_palette):
        self.new_palette = new_palette

    ## GimpPlugIn virtual methods ##
    def do_query_procedures(self):
        # Localization
        self.set_translation_domain ("gimp30-python",
                                     Gio.file_new_for_path(Gimp.locale_directory()))

        return [ "python-fu-palette-offset" ]

    def do_create_procedure(self, name):
        procedure = Gimp.Procedure.new(self, name,
                                       Gimp.PDBProcType.PLUGIN,
                                       self.run, None)
        if name == 'python-fu-palette-offset':
            procedure.set_menu_label(N_("_Offset Palette..."))
            procedure.set_documentation(N_("Offset the colors in a palette"),
                                        "palette_offset (palette, amount) -> modified_palette",
                                        "")
            procedure.set_attribution("Joao S. O. Bueno Calligaris, Carol Spears",
                                      "(c) Joao S. O. Bueno Calligaris",
                                      "2004, 2006")
            procedure.add_argument_from_property(self, "run-mode")
            procedure.add_argument_from_property(self, "palette")
            procedure.add_argument_from_property(self, "amount")
            procedure.add_return_value_from_property(self, "new-palette")
            procedure.add_menu_path ('<Palettes>')
        else:
            procedure = None

        return procedure

    def run(self, procedure, args, data):
        palette = None
        amount  = 1

        # Get the parameters
        if args.length() < 1:
            error = 'No parameters given'
            return procedure.new_return_values(Gimp.PDBStatusType.CALLING_ERROR,
                                               GLib.Error(error))
        runmode = args.index(0)

        if args.length() > 1:
            palette = args.index(1)
        if palette == '' or palette is None:
            palette = Gimp.context_get_palette()
        (exists, num_colors) = Gimp.palette_get_info(palette)
        if not exists:
            error = 'Unknown palette: {}'.format(palette)
            return procedure.new_return_values(Gimp.PDBStatusType.CALLING_ERROR,
                                               GLib.Error(error))

        if args.length() > 2:
            amount = args.index(2)

        if runmode == Gimp.RunMode.INTERACTIVE:
            gi.require_version('Gtk', '3.0')
            from gi.repository import Gtk

            Gimp.ui_init ("palette-offset.py")

            use_header_bar = Gtk.Settings.get_default().get_property("gtk-dialogs-use-header")
            dialog = Gimp.Dialog(use_header_bar=use_header_bar,
                                 title=_("Offset Palette..."))

            dialog.add_button("_Cancel", Gtk.ResponseType.CANCEL)
            dialog.add_button("_OK", Gtk.ResponseType.OK)

            box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL,
                          homogeneous=False, spacing=12)
            dialog.get_content_area().add(box)
            box.show()

            label = Gtk.Label.new_with_mnemonic("Off_set")
            box.pack_start(label, False, False, 1)
            label.show()

            amount = self.set_property("amount", amount)
            spin = Gimp.prop_spin_button_new(self, "amount", 1.0, 5.0, 0)
            spin.set_activates_default(True)
            box.pack_end(spin, False, False, 1)
            spin.show()

            dialog.show()
            if dialog.run() != Gtk.ResponseType.OK:
                return procedure.new_return_values(Gimp.PDBStatusType.CANCEL,
                                                   GLib.Error())
            amount = self.get_property("amount")

        #If palette is read only, work on a copy:
        editable = Gimp.palette_is_editable(palette)
        if not editable:
            palette = Gimp.palette_duplicate (palette)

        tmp_entry_array = []
        for i in range (num_colors):
            tmp_entry_array.append  ((Gimp.palette_entry_get_name (palette, i)[1],
                                      Gimp.palette_entry_get_color (palette, i)[1]))
        for i in range (num_colors):
            target_index = i + amount
            if target_index >= num_colors:
                target_index -= num_colors
            elif target_index < 0:
                target_index += num_colors
            Gimp.palette_entry_set_name (palette, target_index, tmp_entry_array[i][0])
            Gimp.palette_entry_set_color (palette, target_index, tmp_entry_array[i][1])

        retval = procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())
        value = GObject.Value(GObject.TYPE_STRING, palette)
        retval.remove(1)
        retval.insert(1, value)
        return retval

Gimp.main(PaletteOffset.__gtype__, sys.argv)
