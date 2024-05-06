#!/usr/bin/env python3

#   Gimp-Python - allows the writing of Gimp plugins in Python.
#   Copyright (C) 2003, 2005  Manish Singh <yosh@gimp.org>
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

import string
import struct
import os.path
import sys

import gi
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp
gi.require_version('GimpUi', '3.0')
from gi.repository import GimpUi
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio

def N_(message): return message
def _(message): return GLib.dgettext(None, message)


escape_table = {
    '&': '&amp;',
    '<': '&lt;',
    '>': '&gt;',
    '"': '&quot;'
}

style_def = """body {
   width: 100%%;
   font-size: %dpx;
   background-color: #000000;
   color: #ffffff;
}
"""

preamble = """<!DOCTYPE html>
<html>
<head>
<title>CSS Color XHTML written by GIMP</title>
%s
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
</head>
<body>
<pre>
"""

postamble = """\n</pre>\n</body>\n</html>\n"""

def export_colorxhtml(procedure, run_mode, image, file, options, metadata, config, data):
    if file is None:
        error = 'No file given'
        return procedure.new_return_values(Gimp.PDBStatusType.CALLING_ERROR,
                                           GLib.Error(error))

    if run_mode == Gimp.RunMode.INTERACTIVE:

        gi.require_version('Gtk', '3.0')
        from gi.repository import Gtk

        GimpUi.init ("file-colorxhtml-export")

        dialog = GimpUi.ProcedureDialog.new(procedure, config, _("Save as colored HTML text..."))

        dialog.fill_frame("file-frame", "source-file", False, "aux-file")
        # Set the characters text field as not enabled when the user chooses a file
        dialog.set_sensitive("characters", True, config, "source-file", True)

        dialog.fill(["characters", "file-frame", "font-size", "separate"])

        if not dialog.run():
            return procedure.new_return_values(Gimp.PDBStatusType.CANCEL,
                                               GLib.Error())

    source_file = config.get_property("source-file")
    characters  = config.get_property("characters")
    size        = config.get_property("font-size");
    separate    = config.get_property("separate")
    aux_file    = config.get_property("aux-file")

    if source_file and aux_file:
        characters = aux_file.get_path()

    #For now, work with a single layer
    layer = image.get_layers()[0]

    width = layer.get_width()
    height = layer.get_height()
    bpp = layer.get_bpp()

    html = open(file.peek_path(), 'w')

    if separate:
        dirname, cssfile = os.path.split(file.peek_path())
        cssfile = os.path.splitext(cssfile)[0] + '.css'
        cssname = os.path.join(dirname, cssfile)

        css = open(cssname, 'w')

    if source_file:
        characters_file = open(characters, 'r')
        chars = characters_file.read()
        characters_file.close()
    else:
        chars = characters

    # Remove unprintable characters from "chars".
    # TODO: This only handles ascii files.  It would be nice to handle unicode
    # files, so this could work for any language.
    goodchars = string.digits + string.ascii_letters + string.punctuation
    badchars = ''.join(chr(i) for i in range(256) if chr(i) not in goodchars)
    allchars = str.maketrans('', '', badchars)
    chars = chars.translate(allchars)

    data = [escape_table.get(c, c) for c in chars]

    if data:
        data.reverse()
    else:
        data = list('X' * 80)

    Gimp.progress_init(_("Saving as colored XHTML"))

    style = style_def % size

    if separate:
        ss = '<link rel="stylesheet" type="text/css" href="%s" />' % cssfile
        css.write(style)
    else:
        ss = '<style type="text/css">\n%s</style>' % style

    html.write(preamble % ss)

    colors = {}
    chars = []

    for y in range(0, height):

        # The characters in "chars" will be used to draw the next row.
        # Lets fill "chars" with data so it has at least enough characters.
        while len(chars) < width:
            chars[0:0] = data

        for x in range(0, width):
            pixel_color = layer.get_pixel(x, y)
            pixel_rgb = pixel_color.get_rgba()
            pixel_tuple = (int(pixel_rgb[0] * 255),
                           int(pixel_rgb[1] * 255),
                           int(pixel_rgb[2] * 255))

            color = '%02x%02x%02x' % pixel_tuple
            style = 'background-color:black; color:#%s;' % color
            char = chars.pop()

            if separate:
                if color not in colors:
                    css.write('span.N%s { %s }\n' % (color, style))
                    colors[color] = 1

                html.write('<span class="N%s">%s</span>' % (color, char))

            else:
                html.write('<span style="%s">%s</span>' % (style, char))

        html.write('\n')

        Gimp.progress_update(y / float(height))

    html.write(postamble)

    html.close()
    if separate:
        css.close()

    return Gimp.ValueArray.new_from_values([
        GObject.Value(Gimp.PDBStatusType, Gimp.PDBStatusType.SUCCESS)
    ])


class ColorXhtml(Gimp.PlugIn):
    ## GimpPlugIn virtual methods ##
    def do_set_i18n(self, procname):
        return True, 'gimp30-python', None

    def do_query_procedures(self):
        return [ 'file-colorxhtml-export' ]

    def do_create_procedure(self, name):
        procedure = None
        if name == 'file-colorxhtml-export':
            procedure = Gimp.ExportProcedure.new(self, name,
                                                 Gimp.PDBProcType.PLUGIN,
                                                 False, export_colorxhtml, None)
            procedure.set_image_types("RGB")
            procedure.set_documentation (
                _("Save as colored HTML text"),
                "Saves the image as colored XHTML text (based on Perl version by Marc Lehmann)",
                name)
            procedure.set_menu_label(_("Colored HTML text"))
            procedure.set_attribution("Manish Singh and Carol Spears",
                                      "(c) GPL V3.0 or later",
                                      "2003")

            procedure.set_extensions ("html,xhtml");

            procedure.add_boolean_argument ("source-file",
                                            _("Rea_d characters from file"),
                                            _("Read characters from file, if true, or use text entry"),
                                            False, GObject.ParamFlags.READWRITE)
            procedure.add_string_argument ("characters", _("Charac_ters"),
                                           _("Characters that will be used as colored pixels."),
                                           "foo", GObject.ParamFlags.READWRITE)
            procedure.add_int_argument ("font-size", _("Fo_nt size in pixels"),
                                        _("Font size in pixels"), 5, 100, 10,
                                        GObject.ParamFlags.READWRITE)
            procedure.add_boolean_argument ("separate", _("_Write a separate CSS file"),
                                            _("Write a separate CSS file"),
                                            False, GObject.ParamFlags.READWRITE)
            #GUI only, used to create a widget to open a file if source-file is enabled
            procedure.add_file_aux_argument ("aux-file", _("Choose File"),
                                             "", GObject.ParamFlags.READWRITE)

        return procedure

Gimp.main(ColorXhtml.__gtype__, sys.argv)
