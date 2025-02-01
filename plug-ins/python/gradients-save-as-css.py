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

        # Fill the dialog.
        config.set_property("gradient", None)
        dialog.fill (["gradient", "file"])

        if not dialog.run():
            dialog.destroy()
            return procedure.new_return_values(Gimp.PDBStatusType.CANCEL, GLib.Error())

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
        return [ 'python-fu-gradient-save-as-css' ]

    def do_create_procedure(self, name):
        procedure = Gimp.Procedure.new(self, name,
                                       Gimp.PDBProcType.PLUGIN,
                                       gradient_css_save, None)
        if name == 'python-fu-gradient-save-as-css':
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
                                             "", True,
                                             None, True, # Default to context.
                                             GObject.ParamFlags.READWRITE)
            procedure.add_file_argument ("file", _("_File"), "",
                                         Gimp.FileChooserAction.SAVE,
                                         False, None, GObject.ParamFlags.READWRITE)
        return procedure

Gimp.main(GradientsSaveAsCSS.__gtype__, sys.argv)
