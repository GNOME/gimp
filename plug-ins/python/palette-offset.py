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
    @GObject.Property(type=Gimp.Palette,
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
    @GObject.Property(type=Gimp.Palette,
                      nick=_("The edited palette"),
                      blurb=_("The newly created palette when read-only, otherwise the input palette"))
    def new_palette(self):
        return self.new_palette

    @new_palette.setter
    def new_palette(self, new_palette):
        self.new_palette = new_palette

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
            procedure.add_argument_from_property(self, "run-mode")
            procedure.add_argument_from_property(self, "palette")
            procedure.add_argument_from_property(self, "amount")
            procedure.add_return_value_from_property(self, "new-palette")
            procedure.add_menu_path ('<Palettes>')
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

            use_header_bar = Gtk.Settings.get_default().get_property("gtk-dialogs-use-header")
            dialog = GimpUi.Dialog(use_header_bar=use_header_bar,
                                   title=_("Offset Palette..."))

            dialog.add_button(_("_Cancel"), Gtk.ResponseType.CANCEL)
            dialog.add_button(_("_OK"), Gtk.ResponseType.OK)

            box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL,
                          homogeneous=False, spacing=12)
            dialog.get_content_area().add(box)
            box.show()

            label = Gtk.Label.new(_("Offset"))
            box.pack_start(label, False, False, 1)
            label.show()

            amount = self.set_property("amount", amount)
            spin = GimpUi.prop_spin_button_new(self, "amount", 1.0, 5.0, 0)
            spin.set_activates_default(True)
            box.pack_end(spin, False, False, 1)
            spin.show()

            dialog.show()
            if dialog.run() != Gtk.ResponseType.OK:
                return procedure.new_return_values(Gimp.PDBStatusType.CANCEL,
                                                   GLib.Error("Canceled"))
            amount = self.get_property("amount")
            config.set_property("amount", amount)

        #If palette is read only, work on a copy:
        editable = palette.is_editable()
        if not editable:
            palette = palette.duplicate()

        num_colors = palette.get_color_count()
        tmp_entry_array = []
        for i in range (num_colors):
            tmp_entry_array.append  ((palette.entry_get_name(i)[1],
                                      palette.entry_get_color(i)[1]))
        for i in range (num_colors):
            target_index = i + amount
            if target_index >= num_colors:
                target_index -= num_colors
            elif target_index < 0:
                target_index += num_colors
            palette.entry_set_name(target_index, tmp_entry_array[i][0])
            palette.entry_set_color(target_index, tmp_entry_array[i][1])

        retval = procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())
        value = GObject.Value(Gimp.Palette, palette)
        retval.remove(1)
        retval.insert(1, value)
        return retval

Gimp.main(PaletteOffset.__gtype__, sys.argv)
