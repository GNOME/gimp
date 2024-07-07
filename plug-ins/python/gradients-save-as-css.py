#!/usr/bin/env python3
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
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.


# Currently this exports all color segments as RGB linear centered segments.
# TODO: Respect gradient alpha, off-center segments, different blending
# functions and HSV colors

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
import time
import sys

def N_(message): return message
def _(message): return GLib.dgettext(None, message)


w3c_template = """background-image: linear-gradient(top, %s);\n"""

color_to_html = lambda r, g, b: "rgb(%d,%d,%d)" % (int(255 * r), int(255 * g), int(255 * b))

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

def gradient_css_save(procedure, config, data):
    runmode = config.get_property("run-mode")

    if runmode == Gimp.RunMode.INTERACTIVE:
        GimpUi.init('python-fu-gradient-save-as-css')
        dialog = GimpUi.ProcedureDialog(procedure=procedure, config=config)
        file   = None

        # Add gradient button
        dialog.fill (["gradient"])

        # UI for the file parameter
        # from histogram-export.py
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL,
                       homogeneous=False, spacing=10)
        dialog.get_content_area().add(vbox)
        vbox.show()

        grid = Gtk.Grid()
        grid.set_column_homogeneous(False)
        grid.set_border_width(10)
        grid.set_column_spacing(10)
        grid.set_row_spacing(10)
        vbox.add(grid)
        grid.show()

        def choose_file(widget):
            if file_chooser_dialog.run() == Gtk.ResponseType.OK:
                if file_chooser_dialog.get_file() is not None:
                    config.set_property("file", file_chooser_dialog.get_file())
                    file_entry.set_text(file_chooser_dialog.get_file().get_path())
            file_chooser_dialog.hide()

        file_chooser_button = Gtk.Button.new_with_mnemonic(label=_("_File..."))
        grid.attach(file_chooser_button, 0, 0, 1, 1)
        file_chooser_button.show()
        file_chooser_button.connect("clicked", choose_file)

        file_entry = Gtk.Entry.new()
        grid.attach(file_entry, 1, 0, 1, 1)
        file_entry.set_width_chars(40)
        file_entry.set_placeholder_text(_("Choose CSS file..."))
        if config.get_property ("file") != None:
            file = config.get_property("file")
        if file is not None:
            file_entry.set_text(file.get_path())
        file_entry.show()

        use_header_bar = Gtk.Settings.get_default().get_property("gtk-dialogs-use-header")
        file_chooser_dialog = Gtk.FileChooserDialog(use_header_bar=use_header_bar,
                                                    title=_("Save as CSS file..."),
                                                    action=Gtk.FileChooserAction.SAVE)
        file_chooser_dialog.add_button(_("_Cancel"), Gtk.ResponseType.CANCEL)
        file_chooser_dialog.add_button(_("_OK"), Gtk.ResponseType.OK)

        #Connect config so reset works on custom widget
        def gradient_reset (*args):
            if len(args) >= 2:
                if config.get_property("file") is not None and config.get_property("file").get_path() != file_entry.get_text():
                    file_entry.set_text(config.get_property("file").get_path())
                else:
                    file_entry.set_text("")

        config.connect("notify::gradient", gradient_reset)
        config.connect("notify::file", gradient_reset)

        if not dialog.run():
            dialog.destroy()
            return procedure.new_return_values(Gimp.PDBStatusType.CANCEL, GLib.Error())
        else:
            file = Gio.file_new_for_path(file_entry.get_text())
            #Save configs for non-connected UI element
            config.set_property ("file", file)

            dialog.destroy()

    gradient = config.get_property("gradient")
    file     = config.get_property("file")

    if file is None:
        error = 'No file given'
        return procedure.new_return_values(Gimp.PDBStatusType.CALLING_ERROR,
                                           GLib.Error(error))

    stops = []
    n_segments = gradient.get_number_of_segments()
    last_stop = None
    for index in range(n_segments):
        lcolor = gradient.segment_get_left_color(index)
        rcolor = gradient.segment_get_right_color(index)
        success, lpos = gradient.segment_get_left_pos(index)
        success, rpos = gradient.segment_get_right_pos(index)

        if lcolor != None and rcolor != None:
            lr, lg, lb, la = lcolor.get_rgba()
            rr, rg, rb, ra = rcolor.get_rgba()
            lstop = color_to_html(lr, lg, lb) + " %d%%" % int(100 * lpos)
            if lstop != last_stop:
                stops.append(lstop)

            rstop = color_to_html(rr, rg, rb) + " %d%%" % int(100 * rpos)

            stops.append(rstop)
            last_stop = rstop

    final_text = w3c_template % ", ".join(stops)

    success, etag = file.replace_contents(bytes(format_text(final_text), encoding='utf-8'),
                                          etag=None,
                                          make_backup=False,
                                          flags=Gio.FileCreateFlags.REPLACE_DESTINATION,
                                          cancellable=None)
    if success:
        return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())
    else:
        return procedure.new_return_values(Gimp.PDBStatusType.EXECUTION_ERROR,
                                           GLib.Error('File saving failed: {}'.format(file.get_path())))

class GradientsSaveAsCSS (Gimp.PlugIn):
    ## GimpPlugIn virtual methods ##
    def do_set_i18n(self, procname):
        return True, 'gimp30-python', None

    def do_query_procedures(self):
        return [ 'gradient-save-as-css' ]

    def do_create_procedure(self, name):
        procedure = Gimp.Procedure.new(self, name,
                                       Gimp.PDBProcType.PLUGIN,
                                       gradient_css_save, None)
        if name == 'gradient-save-as-css':
            procedure.set_documentation (_("Creates a new palette from a given gradient"),
                                         _("Creates a new palette from a given gradient"),
                                         name)
            procedure.set_menu_label(_("Save Gradient as CSS..."))
            procedure.set_attribution("Joao S. O. Bueno",
                                      "(c) GPL V3.0 or later",
                                      "2011")
            procedure.add_menu_path('<Gradients>/Gradients Menu')

            procedure.add_enum_argument ("run-mode", _("Run mode"),
                                         _("The run mode"), Gimp.RunMode,
                                         Gimp.RunMode.NONINTERACTIVE,
                                         GObject.ParamFlags.READWRITE)
            procedure.add_gradient_argument ("gradient", _("_Gradient to use"),
                                             "", True, GObject.ParamFlags.READWRITE)
            procedure.add_file_argument ("file", _("_File"),
                                         "", GObject.ParamFlags.READWRITE)
        return procedure

Gimp.main(GradientsSaveAsCSS.__gtype__, sys.argv)
