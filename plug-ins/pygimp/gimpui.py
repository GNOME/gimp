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

import gtk, gimp

def _callbackWrapper(menu_item, callback, data):
	callback(menu_item.get_data("Gimp-ID"), data)

def _createMenu(items, callback, data):
	menu = gtk.GtkMenu()
	if not items:
		items = [("(none)", None)]
	for label, id in items:
		menu_item = gtk.GtkMenuItem(label)
		menu_item.set_data("Gimp-ID", id)
		menu.add(menu_item)
		if callback:
			menu_item.connect("activate", _callbackWrapper,
					  callback, data)
		menu_item.show()
	return menu
	

def ImageMenu(constraint=None, callback=None, data=None):
	items = []
	for img in gimp.query_images():
		if constraint and not constraint(img):
			continue
		items.append((img.filename, img))
	items.sort()
	return _createMenu(items, callback, data)

def LayerMenu(constraint=None, callback=None, data=None):
	items = []
	for img in gimp.query_images():
		filename = img.filename
		for layer in img.layers:
			if constraint and not constraint(img, layer):
				continue
			name = filename + "/" + layer.name
			items.append((name, layer))
	items.sort()
	return _createMenu(items, callback, data)

def ChannelMenu(constraint=None, callback=None, data=None):
	items = []
	for img in gimp.query_images():
		filename = img.filename
		for channel in img.channels:
			if constraint and not constraint(img, channel):
				continue
			name = filename + "/" + channel.name
			items.append((name, channel))
	items.sort()
	return _createMenu(items, callback, data)

def DrawableMenu(constraint=None, callback=None, data=None):
	items = []
	for img in gimp.query_images():
		filename = img.filename
		for drawable in img.layers + img.channels:
			if constraint and not constraint(img, drawable):
				continue
			name = filename + "/" + drawable.name
			items.append((name, drawable))
	items.sort()
	return _createMenu(items, callback, data)

class ImageSelector(gtk.GtkOptionMenu):
	def __init__(self, default=None):
		gtk.GtkOptionMenu.__init__(self)
		self.menu = ImageMenu(None, self.clicked)
		self.set_menu(self.menu)
		self.selected = default
		children = self.menu.children()
		for child in range(len(children)):
			if children[child].get_data("Gimp-ID") == default:
				self.set_history(child)
				break
	def clicked(self, img, data=None):
		self.selected = img
	def get_value(self):
		return self.selected
	
class LayerSelector(gtk.GtkOptionMenu):
	def __init__(self, default=None):
		gtk.GtkOptionMenu.__init__(self)
		self.menu = LayerMenu(None, self.clicked)
		self.set_menu(self.menu)
		self.selected = default
		children = self.menu.children()
		for child in range(len(children)):
			if children[child].get_data("Gimp-ID") == default:
				self.set_history(child)
				break
	def clicked(self, layer, data=None):
		self.selected = layer
	def get_value(self):
		return self.selected
	
class ChannelSelector(gtk.GtkOptionMenu):
	def __init__(self, default=None):
		gtk.GtkOptionMenu.__init__(self)
		self.menu = ChannelMenu(None, self.clicked)
		self.set_menu(self.menu)
		self.selected = default
		children = self.menu.children()
		for child in range(len(children)):
			if children[child].get_data("Gimp-ID") == default:
				self.set_history(child)
				break
	def clicked(self, channel, data=None):
		self.selected = channel
	def get_value(self):
		return self.selected
	
class DrawableSelector(gtk.GtkOptionMenu):
	def __init__(self, default=None):
		gtk.GtkOptionMenu.__init__(self)
		self.menu = DrawableMenu(None, self.clicked)
		self.set_menu(self.menu)
		self.selected = default
		children = self.menu.children()
		for child in range(len(children)):
			if children[child].get_data("Gimp-ID") == default:
				self.set_history(child)
				break
	def clicked(self, drawable, data=None):
		self.selected = drawable
	def get_value(self):
		return self.selected

class ColourSelector(gtk.GtkButton):
	def __init__(self, default=(255, 0, 0)):
		gtk.GtkButton.__init__(self)
		self.set_usize(100, 20)

		self.colour = default
		self.update_colour()

		self.dialog = None
		self.connect("clicked", self.show_dialog)
	def update_colour(self):
		r, g, b = self.colour
		colour = self.get_colormap().alloc(r*256, g*256, b*256)
		style = self.get_style().copy()
		style.bg[gtk.STATE_NORMAL] = colour
		style.bg[gtk.STATE_PRELIGHT] = colour
		self.set_style(style)
		self.queue_draw()

	def show_dialog(self, button):
		if self.dialog:
			self.dialog.show()
			return
		self.dialog = gtk.GtkColorSelectionDialog("Colour")
		self.dialog.colorsel.set_color(tuple(map(lambda x: x/255.0,
						   self.colour)))
		def delete_event(win, event):
			win.hide()
			return gtk.TRUE
		self.dialog.connect("delete_event", delete_event)
		self.dialog.ok_button.connect("clicked", self.selection_ok)
		self.dialog.cancel_button.connect("clicked", self.dialog.hide)
		self.dialog.show()

	def selection_ok(self, button):
		colour = self.dialog.colorsel.get_color()
		self.colour = tuple(map(lambda x: int(x*255.99), colour))
		self.update_colour()
		self.dialog.hide()

	def get_value(self):
		return self.colour

class _Selector(gtk.GtkHBox):
	def __init__(self):
		gtk.GtkHBox.__init__(self, gtk.FALSE, 5)
		self.entry = gtk.GtkEntry()
		self.pack_start(self.entry)
		self.entry.show()
		self.button = gtk.GtkButton("...")
		self.button.connect("clicked", self.show_dialog)
		self.pack_start(self.button, expand=gtk.FALSE)
		self.button.show()

		self.dialog = gtk.GtkDialog()
		self.dialog.set_title(self.get_title())
		def delete_event(win, event):
			win.hide()
			return gtk.TRUE
		self.dialog.connect("delete_event", delete_event)

		box = gtk.GtkVBox()
		box.set_border_width(5)
		self.dialog.vbox.pack_start(box)
		box.show()

		swin = gtk.GtkScrolledWindow()
		swin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		box.pack_start(swin)
		swin.show()
		
		items = map(None, self.get_list())
		list = gtk.GtkList()
		list.set_selection_mode(gtk.SELECTION_BROWSE)
		self.selected = self.get_default()
		self.entry.set_text(self.selected)
		items.sort()
		for s in items:
			item = gtk.GtkListItem(s)
			list.add(item)
			if s == self.selected:
				list.select_child(item)
			item.show()
		swin.add_with_viewport(list)
		list.show()

		b = gtk.GtkButton("OK")
		self.dialog.action_area.pack_start(b)
		b.set_flags(gtk.CAN_DEFAULT)
		b.grab_default()
		b.show()
		b.connect("clicked", self.selection_ok, list)

		b = gtk.GtkButton("Cancel")
		self.dialog.action_area.pack_start(b)
		b.set_flags(gtk.CAN_DEFAULT)
		b.show()
		b.connect("clicked", self.dialog.hide)

		self.dialog.set_usize(300, 225)

	def show_dialog(self, button):
		self.dialog.show()
		
	def selection_ok(self, button, list):
		self.dialog.hide()

		sel = list.get_selection()
		if not sel: return
		self.selected = sel[0].children()[0].get()
		self.entry.set_text(self.selected)

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
		num, patterns = gimp.pdb.gimp_patterns_list()
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
		num, brushes = gimp.pdb.gimp_brushes_list()
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
		num, gradients = gimp.pdb.gimp_gradients_get_list()
		return gradients

class FontSelector(gtk.GtkHBox):
	def __init__(self, default="fixed"):
		gtk.GtkHBox.__init__(self, gtk.FALSE, 5)
		self.entry = gtk.GtkEntry()
		self.pack_start(self.entry)
		self.entry.show()
		self.button = gtk.GtkButton("...")
		self.button.connect("clicked", self.show_dialog)
		self.pack_start(self.button, expand=gtk.FALSE)
		self.button.show()

		self.dialog = gtk.GtkFontSelectionDialog("Fonts")
		self.dialog.set_default_size(400, 300)
		def delete_event(win, event):
			win.hide()
			return gtk.TRUE
		self.dialog.connect("delete_event", delete_event)

		self.dialog.ok_button.connect("clicked", self.selection_ok)
		self.dialog.cancel_button.connect("clicked", self.dialog.hide)

		self.dialog.set_font_name(default)
		self.selected = default
		self.entry.set_text(self.selected)
		
	def show_dialog(self, button):
		self.dialog.show()
		
	def selection_ok(self, button):
		self.dialog.hide()
		self.selected = self.dialog.get_font_name()
		self.entry.set_text(self.selected)

	def get_value(self):
		return self.selected
		
class FileSelector(gtk.GtkHBox):
	def __init__(self, default=""):
		gtk.GtkHBox.__init__(self, gtk.FALSE, 5)
		self.entry = gtk.GtkEntry()
		self.pack_start(self.entry)
		self.entry.show()
		self.button = gtk.GtkButton("...")
		self.button.connect("clicked", self.show_dialog)
		self.pack_start(self.button, expand=gtk.FALSE)
		self.button.show()

		self.dialog = gtk.GtkFileSelection("Fonts")
		self.dialog.set_default_size(400, 300)
		def delete_event(win, event):
			win.hide()
			return gtk.TRUE
		self.dialog.connect("delete_event", delete_event)

		self.dialog.ok_button.connect("clicked", self.selection_ok)
		self.dialog.cancel_button.connect("clicked", self.dialog.hide)

		self.dialog.set_filename(default)
		self.selected = self.dialog.get_filename()
		self.entry.set_text(self.selected)
		
	def show_dialog(self, button):
		self.dialog.show()
		
	def selection_ok(self, button):
		self.dialog.hide()
		self.selected = self.dialog.get_filename()
		self.entry.set_text(self.selected)

	def get_value(self):
		return self.selected
