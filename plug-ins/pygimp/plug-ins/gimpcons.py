#!/usr/bin/env python

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
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

from gimpfu import *
import gtkcons

def extension_python_fu_console():
	import gtk, gimpenums, gimpshelf
	gtk.rc_parse(gimp.gtkrc())
	namespace = {'__builtins__': __builtins__,
		     '__name__': '__main__', '__doc__': None,
		     'gimp': gimp, 'pdb': gimp.pdb,
		     'shelf': gimpshelf.shelf}
	for s in gimpenums.__dict__.keys():
		if s[0] != '_':
			namespace[s] = getattr(gimpenums, s)

	win = gtk.GtkWindow()
	win.connect("destroy", gtk.mainquit)
	win.set_title("Gimp-Python Console")
	cons = gtkcons.Console(namespace=namespace,
		copyright='Gimp Python Extensions - Copyright (C), 1997-1999' +
		' James Henstridge\n', quit_cb=gtk.mainquit)

	def browse(button, cons):
		import gtk, pdbbrowse
		def ok_clicked(button, browse, cons=cons):
			cons.line.set_text(browse.cmd)
			browse.destroy()
		win = pdbbrowse.BrowseWin(ok_button=ok_clicked)
		win.connect("destroy", gtk.mainquit)
		win.set_modal(TRUE)
		win.show()
		gtk.mainloop()
	button = gtk.GtkButton("Browse")
	button.connect("clicked", browse, cons)
	cons.inputbox.pack_end(button, expand=FALSE)
	button.show()
	win.add(cons)
	cons.show()
	win.set_default_size(475, 300)
	win.show()
	cons.init()
	# flush the displays every half second
	def timeout():
		gimp.displays_flush()
		return TRUE
	gtk.timeout_add(500, timeout)
	gtk.mainloop()

register(
	"python_fu_console",
	"Python interactive interpreter with gimp extensions",
	"Type in commands and see results",
	"James Henstridge",
	"James Henstridge",
	"1997-1999",
	"<Toolbox>/Xtns/Python-Fu/Console",
	"*",
	[],
	[],
	extension_python_fu_console)

main()

