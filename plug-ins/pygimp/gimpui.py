#   Gimp-Python - allows the writing of Gimp plugins in Python.
#   Copyright (C) 1997  James Henstridge <james@daa.com.au>
#
#    This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

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

import gtk, gobject, gimp

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

class ImageSelector(gtk.OptionMenu):
    def __init__(self, default=None):
        gtk.OptionMenu.__init__(self)
        self.menu = ImageMenu(None, self.clicked)
        self.set_menu(self.menu)
        if default == None:
            self.selected = self.menu.get_children()[0].get_data("Gimp-ID")
            self.set_history (0)
        else:
            self.selected = default
            children = self.menu.get_children()
            for child in range(len(children)):
                if children[child].get_data("Gimp-ID") == default:
                    self.set_history(child)
                    break
    def clicked(self, img, data=None):
        self.selected = img
    def get_value(self):
        return self.selected

class LayerSelector(gtk.OptionMenu):
    def __init__(self, default=None):
        gtk.OptionMenu.__init__(self)
        self.menu = LayerMenu(None, self.clicked)
        self.set_menu(self.menu)
        if default == None:
            self.selected = self.menu.get_children()[0].get_data("Gimp-ID")
            self.set_history (0)
        else:
            self.selected = default
            children = self.menu.get_children()
            for child in range(len(children)):
                if children[child].get_data("Gimp-ID") == default:
                    self.set_history(child)
                    break
    def clicked(self, layer, data=None):
        self.selected = layer
    def get_value(self):
        return self.selected

class ChannelSelector(gtk.OptionMenu):
    def __init__(self, default=None):
        gtk.OptionMenu.__init__(self)
        self.menu = ChannelMenu(None, self.clicked)
        self.set_menu(self.menu)
        if default == None:
            self.selected = self.menu.get_children()[0].get_data("Gimp-ID")
            self.set_history (0)
        else:
            self.selected = default
            children = self.menu.get_children()
            for child in range(len(children)):
                if children[child].get_data("Gimp-ID") == default:
                    self.set_history(child)
                    break
    def clicked(self, channel, data=None):
        self.selected = channel
    def get_value(self):
        return self.selected

class DrawableSelector(gtk.OptionMenu):
    def __init__(self, default=None):
        gtk.OptionMenu.__init__(self)
        self.menu = DrawableMenu(None, self.clicked)
        self.set_menu(self.menu)
        if default == None:
            self.selected = self.menu.get_children()[0].get_data("Gimp-ID")
            self.set_history (0)
        else:
            self.selected = default
            children = self.menu.get_children()
            for child in range(len(children)):
                if children[child].get_data("Gimp-ID") == default:
                    self.set_history(child)
                    break
    def clicked(self, drawable, data=None):
        self.selected = drawable
    def get_value(self):
        return self.selected

class ColourSelector(gtk.Button):
    def __init__(self, default=(255, 0, 0)):
        gtk.Button.__init__(self)
        self.set_size_request(100, 20)

        self.colour = self.get_colormap().alloc_color(*map(lambda x: x*0x101,
                                                           default))
        self.update_colour()

        self.connect("clicked", self.show_dialog)

    def update_colour(self):
        style = self.get_style().copy()
        style.bg[gtk.STATE_NORMAL] = self.colour
        style.bg[gtk.STATE_PRELIGHT] = self.colour
        self.set_style(style)
        self.queue_draw()

    def show_dialog(self, button):
        dialog = gtk.ColorSelectionDialog("Colour")
        dialog.colorsel.set_current_color(self.colour)
        dialog.show()
        response = dialog.run()
        if response == gtk.RESPONSE_OK:
            self.colour = dialog.colorsel.get_current_color()
            self.update_colour()
        dialog.destroy()
            
    def get_value(self):
        return (self.colour.red/257, self.colour.green/257,
                self.colour.blue/257)

class _Selector(gtk.HBox):
    def __init__(self):
        gtk.HBox.__init__(self, False, 5)
        self.entry = gtk.Entry()
        self.pack_start(self.entry)
        self.entry.show()
        self.button = gtk.Button("Browse...")
        self.button.connect("clicked", self.show_dialog)
        self.pack_start(self.button, expand=False)
        self.button.show()

        self.dialog = gtk.Dialog(self.get_title(), None, 0,
                                 (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                  gtk.STOCK_OK, gtk.RESPONSE_OK))

        swin = gtk.ScrolledWindow()
        swin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        self.dialog.vbox.pack_start(swin)
        swin.show()
                
        items = map(None, self.get_list())

        store = gtk.ListStore(gobject.TYPE_STRING)
        treeview = gtk.TreeView(store)

        treeview.set_headers_visible(False)
        column = gtk.TreeViewColumn('Items', gtk.CellRendererText(), text=0)
        treeview.append_column(column)

        self.tv_selection = treeview.get_selection()

        self.selected = self.get_default()
        self.entry.set_text(self.selected)

        sel_path = None
        items.sort()

        for s in items:
            iter = store.append()
            store.set(iter, 0, s)

            if s == self.selected:
                sel_path = store.get_path(iter)

        if sel_path:
            self.tv_selection.select_path(sel_path)

        swin.add(treeview)
        treeview.show()

        self.dialog.set_default_size(300, 225)

    def show_dialog(self, button):
        self.dialog.show()
        response = self.dialog.run()
        self.dialog.hide()

        if response == gtk.RESPONSE_OK:
            self.selected = self.get_selection()
            self.entry.set_text(self.selected)

    def get_selection(self):
        store, iter = self.tv_selection.get_selected()
        return store.get_value(iter, 0)

    def get_value(self):
        return self.selected

class PatternSelector(_Selector):
    def __init__(self, default=""):
        self.default = default
        _Selector.__init__(self)
    def get_default(self):
        return self.default
    def get_title(self):
        return "Patterns"
    def get_list(self):
        num, patterns = gimp.pdb.gimp_patterns_get_list(None)
        return patterns

class BrushSelector(_Selector):
    def __init__(self, default=""):
        self.default = default
        _Selector.__init__(self)
    def get_default(self):
        return self.default
    def get_title(self):
        return "Brushes"
    def get_list(self):
        num, brushes = gimp.pdb.gimp_brushes_get_list(None)
        return brushes

class GradientSelector(_Selector):
    def __init__(self, default=""):
        self.default = default
        _Selector.__init__(self)
    def get_default(self):
        return self.default
    def get_title(self):
        return "Gradients"
    def get_list(self):
        num, gradients = gimp.pdb.gimp_gradients_get_list(None)
        return gradients

class PaletteSelector(_Selector):
    def __init__(self, default=""):
        self.default = default
        _Selector.__init__(self)
    def get_default(self):
        return self.default
    def get_title(self):
        return "Palettes"
    def get_list(self):
        num, palettes = gimp.pdb.gimp_palettes_get_list("")
        return palettes    
    
class FontSelector(gtk.HBox):
    def __init__(self, default="Sans"):
        gtk.HBox.__init__(self, False, 5)
        self.entry = gtk.Entry()
        self.pack_start(self.entry)
        self.entry.show()
        self.button = gtk.Button("Browse...")
        self.button.connect("clicked", self.show_dialog)
        self.pack_start(self.button, expand=False)
        self.button.show()

        self.dialog = gtk.FontSelectionDialog("Fonts")
        self.dialog.set_default_size(400, 300)
        def delete_event(win, event):
            win.hide()
            return True
        self.dialog.connect("delete_event", delete_event)

        self.dialog.set_font_name(default)
        self.selected = default
        self.entry.set_text(self.selected)
        
    def show_dialog(self, button):
        self.dialog.show()
        response = self.dialog.run()
        self.dialog.hide()

        if response == gtk.RESPONSE_OK:
            self.selected = self.dialog.get_font_name()
            self.entry.set_text(self.selected)

    def get_value(self):
        return self.selected

class FileSelector(gtk.HBox):
    def __init__(self, default=""):
        gtk.HBox.__init__(self, False, 5)
        self.button = gtk.FileChooserButton("Pick a file")
        self.pack_start(self.button, False, False, 6)
        self.button.set_filename (default)
        self.button.show()

    def show_dialog(self, button):
        self.dialog.show()
        response = self.dialog.run()
        self.dialog.hide()

        if response == gtk.RESPONSE_OK:
            self.selected = self.dialog.get_filename()
            self.entry.set_text(self.selected)

    def get_value(self):
        return self.button.get_filename ()
