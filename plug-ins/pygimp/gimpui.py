#   Gimp-Python - allows the writing of Gimp plugins in Python.
#   Copyright (C) 1997  James Henstridge <james@daa.com.au>
#
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

'''This module implements the UI items found in the libgimpui library.
It requires pygtk to work.  These functions take use to callbacks -- one
is a constraint function, and the other is the callback object.  The
constraint function takes an image object as its first argument, and
a drawable object as its second if appropriate.  The callback functions
get the selected object as their first argument, and the user data as
the second.

It also implements a number of selector widgets, which can be used to select
various gimp data types.  Each of these selectors takes default as an argument
to the constructor, and has a get_value() method for retrieving the result.
'''

import pygtk
pygtk.require('2.0')

import gtk, gobject, gimp, gimpcolor

from _gimpui import *

import gettext
t = gettext.translation('gimp20-python', gimp.locale_directory, fallback=True)
_ = t.ugettext

def _callbackWrapper(menu_item, callback, data):
    callback(menu_item.get_data("Gimp-ID"), data)

def _createMenu(items, callback, data):
    menu = gtk.Menu()
    if not items:
        items = [("(none)", None)]
    for label, id in items:
        menu_item = gtk.MenuItem(label)
        menu_item.set_data("Gimp-ID", id)
        menu.add(menu_item)
        if callback:
            menu_item.connect("activate", _callbackWrapper,
                              callback, data)
        menu_item.show()
    return menu


def ImageMenu(constraint=None, callback=None, data=None):
    items = []
    for img in gimp.image_list():
        if constraint and not constraint(img):
            continue
        if not img.filename:
            filename = img.name
        else:
            filename = img.filename
        items.append((filename, img))
    items.sort()
    return _createMenu(items, callback, data)

def LayerMenu(constraint=None, callback=None, data=None):
    items = []
    for img in gimp.image_list():
        filename = img.filename
        if not filename:
            filename = img.name
        for layer in img.layers:
            if constraint and not constraint(img, layer):
                continue
            name = filename + "/" + layer.name
            items.append((name, layer))
    items.sort()
    return _createMenu(items, callback, data)

def ChannelMenu(constraint=None, callback=None, data=None):
    items = []
    for img in gimp.image_list():
        filename = img.filename
        if not filename:
            filename = img.name
        for channel in img.channels:
            if constraint and not constraint(img, channel):
                continue
            name = filename + "/" + channel.name
            items.append((name, channel))
    items.sort()
    return _createMenu(items, callback, data)

def DrawableMenu(constraint=None, callback=None, data=None):
    items = []
    for img in gimp.image_list():
        filename = img.filename
        if not filename:
            filename = img.name
        for drawable in img.layers + img.channels:
            if constraint and not constraint(img, drawable):
                continue
            name = filename + "/" + drawable.name
            items.append((name, drawable))
    items.sort()
    return _createMenu(items, callback, data)

def VectorsMenu(constraint=None, callback=None, data=None):
    items = []
    for img in gimp.image_list():
        filename = img.filename
        if not filename:
            filename = img.name
        for vectors in img.vectors:
            if constraint and not constraint(img, vectors):
                continue
            name = filename + "/" + vectors.name
            items.append((name, vectors))
    items.sort()
    return _createMenu(items, callback, data)

class ImageSelector(ImageComboBox):
    def __init__(self, default=None):
        ImageComboBox.__init__(self)
        if default is not None:
            self.set_active_image(default)
    def get_value(self):
        return self.get_active_image()

class LayerSelector(LayerComboBox):
    def __init__(self, default=None):
        LayerComboBox.__init__(self)
        if default is not None:
            self.set_active_layer(default)
    def get_value(self):
        return self.get_active_layer()

class ChannelSelector(ChannelComboBox):
    def __init__(self, default=None):
        ChannelComboBox.__init__(self)
        if default is not None:
            self.set_active_channel(default)
    def get_value(self):
        return self.get_active_channel()

class DrawableSelector(DrawableComboBox):
    def __init__(self, default=None):
        DrawableComboBox.__init__(self)
        if default is not None:
            self.set_active_drawable(default)
    def get_value(self):
        return self.get_active_drawable()

class VectorsSelector(VectorsComboBox):
    def __init__(self, default=None):
        VectorsComboBox.__init__(self)
        if default is not None:
            self.set_active_vectors(default)
    def get_value(self):
        return self.get_active_vectors()

class ColorSelector(ColorButton):
    def __init__(self, default=gimpcolor.RGB(1.0, 0, 0)):
        if isinstance(default, gimpcolor.RGB):
            color = default
        elif isinstance(default, tuple):
            color = apply(gimpcolor.RGB, default)
        elif isinstance(default, str):
            color = gimpcolor.rgb_parse_css(default)
        ColorButton.__init__(self, _("Python-Fu Color Selection"), 100, 20,
                             color, COLOR_AREA_FLAT)
    def get_value(self):
        return self.get_color();

class PatternSelector(PatternSelectButton):
    def __init__(self, default=""):
        PatternSelectButton.__init__(self)
        if default:
            self.set_pattern(default)
    def get_value(self):
        return self.get_pattern()

class BrushSelector(BrushSelectButton):
    def __init__(self, default=""):
        BrushSelectButton.__init__(self)
        if default:
            self.set_brush(default, -1.0, -1, -1)
    def get_value(self):
        return self.get_brush()[0]

class GradientSelector(GradientSelectButton):
    def __init__(self, default=""):
        GradientSelectButton.__init__(self)
        if default:
            self.set_gradient(default)
    def get_value(self):
        return self.get_gradient()

class PaletteSelector(PaletteSelectButton):
    def __init__(self, default=""):
        PaletteSelectButton.__init__(self)
        if default:
            self.set_palette(default)
    def get_value(self):
        return self.get_palette()

class FontSelector(FontSelectButton):
    def __init__(self, default="Sans"):
        FontSelectButton.__init__(self)
        if default:
            self.set_font(default)
    def get_value(self):
        return self.get_font()

class FileSelector(gtk.FileChooserButton):
    def __init__(self, default=""):
        gtk.FileChooserButton.__init__(self, _("Python-Fu File Selection"))
        if default:
            self.set_filename(default)
    def get_value(self):
        return self.get_filename()
