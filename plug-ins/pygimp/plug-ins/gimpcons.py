#!/usr/bin/env python

#   Gimp-Python - allows the writing of Gimp plugins in Python.
#   Copyright (C) 1997  James Henstridge <james@daa.com.au>
#
#   This program is free software; you can redistribute it and/or modify
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

from gimpfu import *

def plug_in_python_fu_console():
    import pygtk
    pygtk.require('2.0')

    import gtk, gimpenums, gimpshelf

    gtk.rc_parse(gimp.gtkrc())

    namespace = {'__builtins__': __builtins__,
                 '__name__': '__main__', '__doc__': None,
                 'gimp': gimp, 'pdb': gimp.pdb,
                 'shelf': gimpshelf.shelf}

    for s in gimpenums.__dict__.keys():
        if s[0] != '_':
            namespace[s] = getattr(gimpenums, s)

    def bye(*args):
        gtk.main_quit()

    win = gtk.Window()
    win.connect("destroy", bye)
    win.set_title("Gimp-Python Console")

    import gtkcons
    cons = gtkcons.Console(namespace=namespace, quit_cb=bye)

    def browse(button, cons):
        import gimpprocbrowser

        def on_apply(proc): 
            cmd = ''

            if len(proc.return_vals) > 0:
                cmd = ', '.join([x[1] for x in proc.return_vals]) + ' = '

            if '-' in proc.proc_name:
                cmd = cmd + "pdb['%s']" % proc.proc_name
            else:
                cmd = cmd + "pdb.%s" % proc.proc_name

            if len(proc.params) > 0 and proc.params[0][1] == 'run_mode':
                params = proc.params[1:]
            else:
                params = proc.params

            cmd = cmd + "(%s)" % ', '.join([x[1] for x in params])

            cons.line.set_text(cmd)
    
        dlg = gimpprocbrowser.dialog_new(on_apply)

    button = gtk.Button("Browse")
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
    gtk.main()

register(
    "python_fu_console",
    "Python interactive interpreter with gimp extensions",
    "Type in commands and see results",
    "James Henstridge",
    "James Henstridge",
    "1997-1999",
    "<Toolbox>/Xtns/Python-Fu/_Console",
    "",
    [],
    [],
    plug_in_python_fu_console)

main()
