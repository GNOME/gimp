#!/usr/bin/env python

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
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

import string
import struct
import inspect
import os.path

import gimp
from gimpfu import *

gettext.install("gimp20-python", gimp.locale_directory, unicode=True)

all_source_types = (CHARS_SOURCE, CHARS_FILE, CHARS_PARAMETER) = range(3)

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

preamble = """<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
                   "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
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

def colorxhtml(img, drawable, filename, raw_filename,
	       source_type, characters, size, separate):
    width = drawable.width
    height = drawable.height
    bpp = drawable.bpp

    if not drawable.is_rgb or drawable.has_alpha:
        return

    if source_type not in all_source_types:
        return

    gimp.tile_cache_ntiles(width / gimp.tile_width() + 1)

    html = file(filename, 'w')

    if separate:
        dirname, cssfile = os.path.split(filename)
        cssfile = os.path.splitext(cssfile)[0] + '.css'
        cssname = os.path.join(dirname, cssfile)

        css = file(cssname, 'w')

    if source_type == CHARS_SOURCE:
        chars = file(inspect.getsourcefile(colorxhtml)).read()
    elif source_type == CHARS_FILE:
        chars = file(characters).read()
    elif source_type == CHARS_PARAMETER:
        chars = characters

    allchars = string.maketrans('', '')

    goodchars = string.digits + string.ascii_letters + string.punctuation
    badchars = ''.join([c for c in allchars if c not in goodchars])

    chars = chars.translate(allchars, badchars)

    data = []

    for c in chars:
        data.append(escape_table.get(c, c))

    if data:
        data.reverse()
    else:
        data = list('X' * 80)

    pr = drawable.get_pixel_rgn(0, 0, width, height, False, False)

    gimp.progress_init(_("Saving as colored XHTML"))

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
        row = pr[0:width, y]

        while len(chars) < width:
            chars[0:0] = data

        for pixel in RowIterator(row, bpp):
            color = '%02x%02x%02x' % pixel
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

        gimp.progress_update(y / float(height))

    html.write(postamble)

    html.close()

    if separate:
        css.close()

def register_save():
    gimp.register_save_handler("file-colorxhtml-save", "xhtml", "")

class RowIterator:
    def __init__(self, row, bpp):
        self.row = row
        self.bpp = bpp

        self.start = 0
        self.stop = bpp

        self.length = len(row)
        self.fmt = 'B' * bpp

    def __iter__(self):
        return iter(self.get_pixel, None)

    def get_pixel(self):
        if self.stop > self.length:
            return None

        pixel = struct.unpack(self.fmt, self.row[self.start:self.stop])

        self.start += self.bpp
        self.stop += self.bpp

        return pixel

register(
    "file-colorxhtml-save",
    N_("Save as colored XHTML"),
    "Saves the image as colored XHTML text (based on Perl version by Marc Lehmann)",
    "Manish Singh and Carol Spears",
    "Manish Singh and Carol Spears",
    "2003",
    N_("Colored XHTML"),
    "RGB",
    [
        (PF_IMAGE, "image", "Input image", None),
        (PF_DRAWABLE, "drawable", "Input drawable", None),
        (PF_STRING, "filename", "The name of the file", None),
        (PF_STRING, "raw-filename", "The name of the file", None),
        (PF_RADIO, "source", _("Character _source"), 0,
                   ((_("Source code"), CHARS_SOURCE),
                    (_("Text file"),   CHARS_FILE),
                    (_("Entry box"),   CHARS_PARAMETER))),
        (PF_FILE,  "characters", _("_File to read or characters to use"),
                   ""),
        (PF_INT,   "font-size",  _("Fo_nt size in pixels"), 10),
        (PF_BOOL,  "separate",   _("_Write a separate CSS file"),   True)
    ],
    [],
    colorxhtml, on_query=register_save,
    menu="<Save>", domain=("gimp20-python", gimp.locale_directory)
    )

main()
