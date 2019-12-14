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
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio

import gettext
_ = gettext.gettext
def N_(message): return message

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

fmt_from_bpp = {
        3: 'BBB',
        6: 'HHH',
        12: 'III'
}

def save_colorxhtml(procedure, run_mode, image, drawable, file, args, data):
    source_file = args.index(0)
    characters = args.index(1)
    size =  args.index(2)
    separate = args.index(3)

    if file is None:
        error = 'No file given'
        return procedure.new_return_values(Gimp.PDBStatusType.CALLING_ERROR,
                                           GLib.Error(error))

    if run_mode == Gimp.RunMode.INTERACTIVE:

        gi.require_version('Gtk', '3.0')
        from gi.repository import Gtk

        Gimp.ui_init ("colorxhtml.py")

        use_header_bar = Gtk.Settings.get_default().get_property("gtk-dialogs-use-header")
        dialog = Gtk.Dialog(use_header_bar=use_header_bar,
                            title=_("Save as colored HTML text..."))
        dialog.add_button("_Cancel", Gtk.ResponseType.CANCEL)
        dialog.add_button("_OK", Gtk.ResponseType.OK)

        choose_file_dialog = Gtk.FileChooserDialog(use_header_bar=use_header_bar,
                                       title=_("Read characters from file..."),
                                       action=Gtk.FileChooserAction.OPEN)
        choose_file_dialog.add_button("_Cancel", Gtk.ResponseType.CANCEL)
        choose_file_dialog.add_button("_OK", Gtk.ResponseType.OK)

        def choose_file(button, user_data=None):
            choose_file_dialog.show()
            if choose_file_dialog.run() == Gtk.ResponseType.OK:
                characters_entry.set_text(choose_file_dialog.get_filename())

            choose_file_dialog.hide()

        grid = Gtk.Grid()
        grid.set_column_homogeneous(False)
        grid.set_border_width(10)
        grid.set_column_spacing(10)
        grid.set_row_spacing(10)

        row = 0
        label = Gtk.Label(label=_("Characters"))
        label.set_tooltip_text(_("Characters that will be used as colored pixels. "))

        grid.attach(label, 0, row, 1 , 1)
        label.show()

        characters_entry = Gtk.Entry()
        characters_entry.set_width_chars(20)
        characters_entry.set_max_width_chars(80)
        characters_entry.set_text(characters)
        characters_entry.set_placeholder_text(_("Characters or file location"))
        grid.attach(characters_entry, 1, row, 1, 1)
        characters_entry.show()

        row += 1

        characters_checkbox = Gtk.CheckButton(label=_("Read characters from file"))
        characters_checkbox.set_active(source_file)
        characters_checkbox.set_tooltip_text(
            _("If set, the Characters text entry will be used as a file name, "
              "from which the characters will be read. Otherwise, the characters "
              "in the text entry will be used to render the image."))
        grid.attach(characters_checkbox, 0, row, 1, 1)
        characters_checkbox.show()

        choose_file_button = Gtk.Button(label=_("Choose file"))
        grid.attach(choose_file_button, 1, row, 1, 1)
        choose_file_button.connect("clicked", choose_file)
        choose_file_button.show()

        row += 1

        label = Gtk.Label(label=_("Font Size(px)"))
        grid.attach(label, 0, row, 1 , 1)
        label.show()

        font_size_adj = Gtk.Adjustment.new(size, 0.0, 100.0, 1.0, 0.0, 0.0)
        font_size_spin = Gtk.SpinButton.new(font_size_adj, climb_rate=1.0, digits=0)
        font_size_spin.set_numeric(True)
        grid.attach(font_size_spin, 1, row, 1 , 1)
        font_size_spin.show()

        row += 1

        separate_checkbox = Gtk.CheckButton(label=_("Write separate CSS file"))
        separate_checkbox.set_active(separate)
        grid.attach(separate_checkbox, 0, row, 2, 1)
        separate_checkbox.show()

        dialog.get_content_area().add(grid)
        grid.show()

        dialog.show()
        if dialog.run() == Gtk.ResponseType.OK:
            separate = separate_checkbox.get_active()
            size = font_size_spin.get_value_as_int()
            source_file = characters_checkbox.get_active()
            characters = characters_entry.get_text()
        else:
            return procedure.new_return_values(Gimp.PDBStatusType.CANCEL,
                                               GLib.Error())

    width = drawable.width()
    height = drawable.height()
    bpp = drawable.bpp()

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

    # Constants used for formatting the pixel color. We can handle image
    # types where each color is 8 bits, 16 bits, or 32 bit integers.
    fmt = fmt_from_bpp[bpp]
    pixel_shift = 8 * (bpp//3 - 1)

    for y in range(0, height):

        # The characters in "chars" will be used to draw the next row.
        # Lets fill "chars" with data so it has at least enough characters.
        while len(chars) < width:
            chars[0:0] = data

        for x in range(0, width):
            pixel_bytes = drawable.get_pixel(x, y)
            pixel_tuple = struct.unpack(fmt, pixel_bytes)
            if bpp > 3:
                 pixel_tuple=(
                   pixel_tuple[0] >> pixel_shift,
                   pixel_tuple[1] >> pixel_shift,
                   pixel_tuple[2] >> pixel_shift,
                 )
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

    retval = Gimp.ValueArray.new(1)
    retval.insert(0, GObject.Value(Gimp.PDBStatusType, Gimp.PDBStatusType.SUCCESS))

    return retval


class ColorXhtml(Gimp.PlugIn):
    ## Parameters ##
    __gproperties__ = {
        "source-file":(bool,
                     _("_Read characters from file, if true, or use text entry"),
                     _("_Read characters from file, if true, or use text entry"),
                      False,
                     GObject.ParamFlags.READWRITE),
        "characters": (str,
                      _("_File to read or characters to use"),
                      _("_File to read or characters to use"),
                      "foo",
                      GObject.ParamFlags.READWRITE),
        "font-size": (int,
                      _("Fo_nt size in pixels"),
                      _("Fo_nt size in pixels"),
                      5, 100, 10,
                      GObject.ParamFlags.READWRITE),
        "separate": (bool,
                     _("_Write a separate CSS file"),
                     _("_Write a separate CSS file"),
                      False,
                     GObject.ParamFlags.READWRITE)
    }

    ## GimpPlugIn virtual methods ##
    def do_query_procedures(self):
        self.set_translation_domain("gimp30-python",
                                    Gio.file_new_for_path(Gimp.locale_directory()))
        return [ 'file-colorxhtml-save' ]

    def do_create_procedure(self, name):
        procedure = None
        if name == 'file-colorxhtml-save':
            procedure = Gimp.SaveProcedure.new(self, name,
                                           Gimp.PDBProcType.PLUGIN,
                                           save_colorxhtml, None)
            procedure.set_image_types("RGB")
            procedure.set_documentation (
                N_("Save as colored HTML text"),
                "Saves the image as colored XHTML text (based on Perl version by Marc Lehmann)",
                name)
            procedure.set_menu_label(N_("Colored HTML text"))
            procedure.set_attribution("Manish Singh and Carol Spears",
                                      "(c) GPL V3.0 or later",
                                      "2003")

            procedure.set_extensions ("html,xhtml");

            procedure.add_argument_from_property(self, "source-file")
            procedure.add_argument_from_property(self, "characters")
            procedure.add_argument_from_property(self, "font-size")
            procedure.add_argument_from_property(self, "separate")

        return procedure

Gimp.main(ColorXhtml.__gtype__, sys.argv)
