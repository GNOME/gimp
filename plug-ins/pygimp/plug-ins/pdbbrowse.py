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
import gimpenums
import gtk
import string

pars = filter(lambda x: x[:4] == 'PDB_', dir(gimpenums))
partypes = [''] * len(pars)
for i in pars:
	partypes[gimpenums.__dict__[i]] = i[4:]
del pars, i

class BrowseWin(gtk.GtkWindow):
	def __init__(self, ok_button=None):
		gtk.GtkWindow.__init__(self)
		self.set_title("PDB Browser")

		vbox = gtk.GtkVBox(FALSE, 5)
		vbox.set_border_width(2)
		self.add(vbox)
		vbox.show()

		paned = gtk.GtkHPaned()
		vbox.pack_start(paned)
		paned.show()

		listsw = gtk.GtkScrolledWindow()
		listsw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		paned.add1(listsw)
		listsw.show()

		self.list = gtk.GtkCList(1)
		self.list.set_column_auto_resize(0, TRUE)
		self.list.set_selection_mode(gtk.SELECTION_BROWSE)
		listsw.add(self.list)
		self.list.show()
		self.update_list()
		self.list.connect("select_row", self.display)

		self.infosw = gtk.GtkScrolledWindow()
		self.infosw.set_policy(gtk.POLICY_AUTOMATIC,
				       gtk.POLICY_AUTOMATIC)
		paned.add2(self.infosw)
		self.infosw.show()

		self.info = None

		paned.set_position(150)
		self.cmd = None
		self.display(self.list, 0, -1, None)

		hbox = gtk.GtkHBox(FALSE, 5)
		vbox.pack_start(hbox, expand=FALSE)
		hbox.show()

		entry = gtk.GtkEntry()
		hbox.pack_start(entry, expand=FALSE)
		entry.show()

		button = gtk.GtkButton("Search by Name")
		button.connect("clicked", self.search_name, entry)
		hbox.pack_start(button, expand=FALSE)
		button.show()
		
		button = gtk.GtkButton("Search by Blurb")
		button.connect("clicked", self.search_blurb, entry)
		hbox.pack_start(button, expand=FALSE)
		button.show()

		button = gtk.GtkButton("Close")
		button.connect("clicked", self.destroy)
		hbox.pack_end(button, expand=FALSE)
		button.show()

		if ok_button:
			button = gtk.GtkButton("OK")
			button.connect("clicked", ok_button, self)
			hbox.pack_end(button, expand=FALSE)
			button.show()

		self.set_default_size(500, 300)

	def search_name(self, button, entry):
		self.update_list(name=entry.get_text())
	def search_blurb(self, button, entry):
		self.update_list(blurb=entry.get_text())

	def update_list(self, name='.*', blurb='.*'):
		self.pdblist = pdb.query(name,blurb,'.*','.*','.*','.*','.*')
		self.pdblist.sort()
		self.list.clear()
		for item in self.pdblist:
			self.list.append([item])
	
	def display(self, clist, row, column, event):
		proc = pdb[clist.get_text(row, 0)]
		self.info = gtk.GtkTable(1, 5, FALSE);
		row = 0
		label = gtk.GtkLabel("Name:")
		label.set_alignment(1.0, 0.5)
		self.info.attach(label, 0,1, row,row+1,
				 xoptions=gtk.FILL, yoptions=gtk.FILL)
		label.show()

		label = gtk.GtkEntry()
		label.set_text(proc.proc_name)
		label.set_editable(FALSE)
		self.info.attach(label, 1,4, row,row+1,
				 xoptions=gtk.FILL, yoptions=gtk.FILL)
		label.show()
		row = row + 1

		label = gtk.GtkLabel("Blurb:")
		label.set_alignment(1.0, 0.5)
		self.info.attach(label, 0,1, row,row+1,
				 xoptions=gtk.FILL, yoptions=gtk.FILL)
		label.show()

		label = gtk.GtkLabel(proc.proc_blurb)
		label.set_alignment(0.0, 0.5)
		self.info.attach(label, 1,4, row,row+1,
				 yoptions=gtk.FILL)
		label.show()
		row = row + 1

		label = gtk.GtkLabel("Copyright:")
		label.set_alignment(1.0, 0.5)
		self.info.attach(label, 0,1, row,row+1,
				 xoptions=gtk.FILL, yoptions=gtk.FILL)
		label.show()

		label = gtk.GtkLabel(proc.proc_date+", "+proc.proc_copyright)
		label.set_alignment(0.0, 0.5)
		self.info.attach(label, 1,4, row,row+1,
				 yoptions=gtk.FILL)
		label.show()
		row = row + 1

		label = gtk.GtkLabel("Author:")
		label.set_alignment(1.0, 0.5)
		self.info.attach(label, 0,1, row,row+1,
				 xoptions=gtk.FILL, yoptions=gtk.FILL)
		label.show()

		label = gtk.GtkLabel(proc.proc_author)
		label.set_alignment(0.0, 0.5)
		self.info.attach(label, 1,4, row,row+1,
				 yoptions=gtk.FILL)
		label.show()
		row = row + 1

		hsep = gtk.GtkHSeparator()
		self.info.attach(hsep, 0,4, row,row+1,
				 yoptions=gtk.FILL)
		hsep.show()
		row = row + 1

		if len(proc.params) > 0:
			label = gtk.GtkLabel("In:")
			label.set_alignment(1.0, 0.5)
			self.info.attach(label, 0,1, row,row+len(proc.params),
					 xoptions=gtk.FILL, yoptions=gtk.FILL)
			label.show()
			for tp, name, desc in proc.params:
				label = gtk.GtkLabel(name)
				label.set_alignment(0.0, 0.5)
				self.info.attach(label, 1,2, row,row+1,
						 xoptions=gtk.FILL,
						 yoptions=gtk.FILL)
				label.show()

				label = gtk.GtkLabel(partypes[tp])
				label.set_alignment(0.0, 0.5)
				self.info.attach(label, 2,3, row,row+1,
						 xoptions=gtk.FILL,
						 yoptions=gtk.FILL)
				label.show()

				label = gtk.GtkLabel(desc)
				label.set_alignment(0.0, 0.5)
				self.info.attach(label, 3,4, row,row+1,
						 yoptions=gtk.FILL)
				label.show()
				row = row + 1
			hsep = gtk.GtkHSeparator()
			self.info.attach(hsep, 0,4, row,row+1,
					 yoptions=gtk.FILL)
			hsep.show()
			row = row + 1
			
		if len(proc.return_vals) > 0:
			label = gtk.GtkLabel("Out:")
			label.set_alignment(1.0, 0.5)
			self.info.attach(label, 0,1,
					 row,row+len(proc.return_vals),
					 xoptions=gtk.FILL, yoptions=gtk.FILL)
			label.show()
			for tp, name, desc in proc.return_vals:
				label = gtk.GtkLabel(name)
				label.set_alignment(0.0, 0.5)
				self.info.attach(label, 1,2, row,row+1,
						 xoptions=gtk.FILL,
						 yoptions=gtk.FILL)
				label.show()

				label = gtk.GtkLabel(partypes[tp])
				label.set_alignment(0.0, 0.5)
				self.info.attach(label, 2,3, row,row+1,
						 xoptions=gtk.FILL,
						 yoptions=gtk.FILL)
				label.show()

				label = gtk.GtkLabel(desc)
				label.set_alignment(0.0, 0.5)
				self.info.attach(label, 3,4, row,row+1,
						 yoptions=gtk.FILL)
				label.show()
				row = row + 1
			hsep = gtk.GtkHSeparator()
			self.info.attach(hsep, 0,4, row,row+1,
					 yoptions=gtk.FILL)
			hsep.show()
			row = row + 1
		
		label = gtk.GtkLabel("Help:")
		label.set_alignment(1.0, 0.5)
		self.info.attach(label, 0,1, row,row+1,
				 xoptions=gtk.FILL, yoptions=gtk.FILL)
		label.show()

		label = gtk.GtkLabel(proc.proc_help)
		label.set_alignment(0.0, 0.5)
		label.set_justify(gtk.JUSTIFY_LEFT)
		label.set_line_wrap(TRUE)
		label.set_usize(300, -1)
		self.info.attach(label, 1,4, row,row+1,
				 yoptions=gtk.FILL)
		label.show()
		row = row + 1

		self.info.set_col_spacings(5)
		self.info.set_row_spacings(3)
		self.info.set_border_width(3)

		children = self.infosw.children()
		if children:
			self.infosw.remove(children[0])

		self.infosw.add_with_viewport(self.info)
		self.info.show()

		# now setup the self.cmd
		self.cmd = ''
		if len(proc.return_vals) > 0:
			self.cmd = string.join(
				map(lambda x: x[1], proc.return_vals), ', ') +\
				' = '
		if '-' in proc.proc_name:
			self.cmd = self.cmd + "pdb['" + proc.proc_name + "']"
		else:
			self.cmd = self.cmd + "pdb." + proc.proc_name
		if len(proc.params) > 0 and proc.params[0][1] == 'run_mode':
			params = proc.params[1:]
		else:
			params = proc.params
		self.cmd = self.cmd + "(" + string.join(
			map(lambda x: x[1], params), ', ') + ")"
		
if __name__ == '__main__':
	def extension_pdb_browse():
		gtk.rc_parse(gimp.gtkrc())
		win = BrowseWin()
		win.connect("destroy", gtk.mainquit)
		win.show()
		gtk.mainloop()
	register(
		"python_fu_pdb_browse",
		"Browse the Procedural Database",
		"Pick a PDB proc, and read the information",
		"James Henstridge",
		"James Henstridge",
		"1997-1999",
		"<Toolbox>/Xtns/Python-Fu/PDB Browser",
		"*",
		[],
		[],
		extension_pdb_browse)
	main()

