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
import string

BrowseWin = None

pars = filter(lambda x: x[:4] == 'PDB_', dir(gimpenums))
partypes = [''] * len(pars)
for i in pars:
    partypes[gimpenums.__dict__[i]] = i[4:]
del pars, i

def define_browse_win():
    import pygtk
    pygtk.require('2.0')

    import gtk

    global BrowseWin
    class BrowseWin(gtk.Window):
        def __init__(self, ok_button=None):
            gtk.Window.__init__(self)
            self.set_title("PDB Browser")

            vbox = gtk.VBox(FALSE, 5)
            vbox.set_border_width(2)
            self.add(vbox)
            vbox.show()

            paned = gtk.HPaned()
            vbox.pack_start(paned)
            paned.show()

            listsw = gtk.ScrolledWindow()
            listsw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
            paned.add1(listsw)
            listsw.show()

            self.list = gtk.CList(1)
            self.list.set_column_auto_resize(0, TRUE)
            self.list.set_selection_mode(gtk.SELECTION_BROWSE)
            listsw.add(self.list)
            self.list.show()
            self.update_list()
            self.list.connect("select_row", self.display)

            self.infosw = gtk.ScrolledWindow()
            self.infosw.set_policy(gtk.POLICY_AUTOMATIC,
                                   gtk.POLICY_AUTOMATIC)
            paned.add2(self.infosw)
            self.infosw.show()

            self.info = None

            paned.set_position(150)
            self.cmd = None
            self.display(self.list, 0, -1, None)

            hbox = gtk.HBox(FALSE, 5)
            vbox.pack_start(hbox, expand=FALSE)
            hbox.show()

            entry = gtk.Entry()
            hbox.pack_start(entry, expand=FALSE)
            entry.show()

            button = gtk.Button("Search by name")
            button.connect("clicked", self.search_name, entry)
            hbox.pack_start(button, expand=FALSE)
            button.show()
                
            button = gtk.Button("Search by blurb")
            button.connect("clicked", self.search_blurb, entry)
            hbox.pack_start(button, expand=FALSE)
            button.show()

            button = gtk.Button("Close")
            button.connect("clicked", lambda btn, win: win.destroy(), self)
            hbox.pack_end(button, expand=FALSE)
            button.show()

            if ok_button:
                button = gtk.Button("OK")
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
            self.info = gtk.Table(1, 5, FALSE);
            row = 0
            label = gtk.Label("Name:")
            label.set_alignment(1.0, 0.5)
            self.info.attach(label, 0,1, row,row+1,
                             xoptions=gtk.FILL, yoptions=gtk.FILL)
            label.show()

            label = gtk.Entry()
            label.set_text(proc.proc_name)
            label.set_editable(FALSE)
            self.info.attach(label, 1,4, row,row+1,
                             xoptions=gtk.FILL, yoptions=gtk.FILL)
            label.show()
            row = row + 1

            label = gtk.Label("Blurb:")
            label.set_alignment(1.0, 0.5)
            self.info.attach(label, 0,1, row,row+1,
                             xoptions=gtk.FILL, yoptions=gtk.FILL)
            label.show()

            label = gtk.Label(proc.proc_blurb)
            label.set_alignment(0.0, 0.5)
            self.info.attach(label, 1,4, row,row+1,
                             yoptions=gtk.FILL)
            label.show()
            row = row + 1

            label = gtk.Label("Copyright:")
            label.set_alignment(1.0, 0.5)
            self.info.attach(label, 0,1, row,row+1,
                             xoptions=gtk.FILL, yoptions=gtk.FILL)
            label.show()

            label = gtk.Label(proc.proc_date+", "+proc.proc_copyright)
            label.set_alignment(0.0, 0.5)
            self.info.attach(label, 1,4, row,row+1,
                             yoptions=gtk.FILL)
            label.show()
            row = row + 1

            label = gtk.Label("Author:")
            label.set_alignment(1.0, 0.5)
            self.info.attach(label, 0,1, row,row+1,
                             xoptions=gtk.FILL, yoptions=gtk.FILL)
            label.show()

            label = gtk.Label(proc.proc_author)
            label.set_alignment(0.0, 0.5)
            self.info.attach(label, 1,4, row,row+1,
                             yoptions=gtk.FILL)
            label.show()
            row = row + 1

            hsep = gtk.HSeparator()
            self.info.attach(hsep, 0,4, row,row+1,
                             yoptions=gtk.FILL)
            hsep.show()
            row = row + 1

            if len(proc.params) > 0:
                label = gtk.Label("In:")
                label.set_alignment(1.0, 0.5)
                self.info.attach(label, 0,1, row,row+len(proc.params),
                                 xoptions=gtk.FILL, yoptions=gtk.FILL)
                label.show()
                for tp, name, desc in proc.params:
                    label = gtk.Label(name)
                    label.set_alignment(0.0, 0.5)
                    self.info.attach(label, 1,2, row,row+1,
                                     xoptions=gtk.FILL,
                                     yoptions=gtk.FILL)
                    label.show()

                    label = gtk.Label(partypes[tp])
                    label.set_alignment(0.0, 0.5)
                    self.info.attach(label, 2,3, row,row+1,
                                     xoptions=gtk.FILL,
                                     yoptions=gtk.FILL)
                    label.show()

                    label = gtk.Label(desc)
                    label.set_alignment(0.0, 0.5)
                    self.info.attach(label, 3,4, row,row+1,
                                     yoptions=gtk.FILL)
                    label.show()
                    row = row + 1
                hsep = gtk.HSeparator()
                self.info.attach(hsep, 0,4, row,row+1,
                                 yoptions=gtk.FILL)
                hsep.show()
                row = row + 1
                        
            if len(proc.return_vals) > 0:
                label = gtk.Label("Out:")
                label.set_alignment(1.0, 0.5)
                self.info.attach(label, 0,1,
                                 row,row+len(proc.return_vals),
                                 xoptions=gtk.FILL, yoptions=gtk.FILL)
                label.show()
                for tp, name, desc in proc.return_vals:
                    label = gtk.Label(name)
                    label.set_alignment(0.0, 0.5)
                    self.info.attach(label, 1,2, row,row+1,
                                     xoptions=gtk.FILL,
                                     yoptions=gtk.FILL)
                    label.show()

                    label = gtk.Label(partypes[tp])
                    label.set_alignment(0.0, 0.5)
                    self.info.attach(label, 2,3, row,row+1,
                                     xoptions=gtk.FILL,
                                     yoptions=gtk.FILL)
                    label.show()

                    label = gtk.Label(desc)
                    label.set_alignment(0.0, 0.5)
                    self.info.attach(label, 3,4, row,row+1,
                                     yoptions=gtk.FILL)
                    label.show()
                    row = row + 1
                hsep = gtk.HSeparator()
                self.info.attach(hsep, 0,4, row,row+1,
                                 yoptions=gtk.FILL)
                hsep.show()
                row = row + 1

            label = gtk.Label("Help:")
            label.set_alignment(1.0, 0.5)
            self.info.attach(label, 0,1, row,row+1,
                             xoptions=gtk.FILL, yoptions=gtk.FILL)
            label.show()

            label = gtk.Label(proc.proc_help)
            label.set_alignment(0.0, 0.5)
            label.set_justify(gtk.JUSTIFY_LEFT)
            label.set_line_wrap(TRUE)
            label.set_size_request(300, -1)
            self.info.attach(label, 1,4, row,row+1,
                             yoptions=gtk.FILL)
            label.show()
            row = row + 1

            self.info.set_col_spacings(5)
            self.info.set_row_spacings(3)
            self.info.set_border_width(3)

            if self.infosw.child:
                self.infosw.remove(self.infosw.child)

            self.infosw.add_with_viewport(self.info)
            self.info.show()

            # now setup the self.cmd
            self.cmd = ''
            if len(proc.return_vals) > 0:
                self.cmd = string.join(
                    map(lambda x: x[1], proc.return_vals), ', ') + ' = '
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
    def plug_in_pdb_browse():
        import pygtk
        pygtk.require('2.0')

        import gtk

        gtk.rc_parse(gimp.gtkrc())

        define_browse_win()

        def bye(*args):
            gtk.main_quit()

        win = BrowseWin()
        win.connect("destroy", bye)
        win.show()

        gtk.main()

    register(
        "python_fu_pdb_browse",
        "Browse the Procedural Database",
        "Pick a PDB proc, and read the information",
        "James Henstridge",
        "James Henstridge",
        "1997-1999",
        "<Toolbox>/Xtns/Python-Fu/_PDB Browser",
        "*",
        [],
        [],
        plug_in_pdb_browse)

    main()

else:
    define_browse_win()
