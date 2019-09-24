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
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio
import time
import sys

import gettext
_ = gettext.gettext
def N_(message): return message

w3c_template = """background-image: linear-gradient(top, %s);\n"""
moz_template = """background-image: -moz-linear-gradient(center top, %s);\n"""
webkit_template = """background-image: -webkit-gradient(linear, """ \
                  """left top, left bottom, %s);\n"""

color_to_html = lambda c: "rgb(%d,%d,%d)" % (c.r, c.g, c.b)

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

def gradient_css_save(procedure, args, data):
    if args.length() != 3:
        error = 'Wrong parameters given'
        return procedure.new_return_values(Gimp.PDBStatusType.CALLING_ERROR,
                                           GLib.Error(error))
    runmode = args.index(0)
    gradient = args.index(1)
    file = args.index(2)

    if runmode == Gimp.RunMode.INTERACTIVE:
        # Interactive mode works on active gradient.
        gradient = Gimp.context_get_gradient()

        # Pop-up a file chooser for target file.
        gi.require_version('Gtk', '3.0')
        from gi.repository import Gtk

        Gimp.ui_init ("gradients-save-as-css.py")

        use_header_bar = Gtk.Settings.get_default().get_property("gtk-dialogs-use-header")
        dialog = Gtk.FileChooserDialog(use_header_bar=use_header_bar,
                                       title=_("CSS file..."),
                                       action=Gtk.FileChooserAction.SAVE)
        dialog.add_button("_Cancel", Gtk.ResponseType.CANCEL)
        dialog.add_button("_OK", Gtk.ResponseType.OK)
        dialog.show()
        if dialog.run() == Gtk.ResponseType.OK:
            file = dialog.get_file()
        else:
            return procedure.new_return_values(Gimp.PDBStatusType.CANCEL,
                                               GLib.Error())

    if file is None:
        error = 'No file given'
        return procedure.new_return_values(Gimp.PDBStatusType.CALLING_ERROR,
                                           GLib.Error(error))

    stops = []
    wk_stops = []
    n_segments = Gimp.gradient_get_number_of_segments(gradient)
    last_stop = None
    for index in range(n_segments):
        success, lcolor, lopacity = Gimp.gradient_segment_get_left_color(gradient, index)
        success, rcolor, ropacity = Gimp.gradient_segment_get_right_color(gradient, index)
        success, lpos = Gimp.gradient_segment_get_left_pos(gradient, index)
        success, rpos = Gimp.gradient_segment_get_right_pos(gradient, index)

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
   ## Parameters ##
    __gproperties__ = {
        "run-mode": (Gimp.RunMode,
                     "Run mode",
                     "The run mode",
                     Gimp.RunMode.NONINTERACTIVE,
                     GObject.ParamFlags.READWRITE),
        "gradient": (str,
                     _("Gradient to use"), "", "",
                     GObject.ParamFlags.READWRITE),
        "file": (Gio.File,
                 _("File"), None,
                 GObject.ParamFlags.READWRITE),
    }

    ## GimpPlugIn virtual methods ##
    def do_query_procedures(self):
        self.set_translation_domain("gimp30-python",
                                    Gio.file_new_for_path(Gimp.locale_directory()))
        return [ 'gradient-save-as-css' ]

    def do_create_procedure(self, name):
        procedure = None
        if name == 'gradient-save-as-css':
            procedure = Gimp.Procedure.new(self, name,
                                           Gimp.PDBProcType.PLUGIN,
                                           gradient_css_save, None)
            procedure.set_image_types("*")
            procedure.set_documentation ("Creates a new palette from a given gradient",
                                         "palette_from_gradient (gradient, number, segment_colors) -> None",
                                         name)
            procedure.set_menu_label("Save as CSS...")
            procedure.set_attribution("Joao S. O. Bueno",
                                      "(c) GPL V3.0 or later",
                                      "2011")
            procedure.add_menu_path('<Gradients>')

            procedure.add_argument_from_property(self, "run-mode")
            procedure.add_argument_from_property(self, "gradient")
            procedure.add_argument_from_property(self, "file")
        return procedure

Gimp.main(GradientsSaveAsCSS.__gtype__, sys.argv)
