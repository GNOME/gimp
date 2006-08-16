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

def pdb_browse():
    import pygtk
    pygtk.require('2.0')

    import gtk
    gtk.rc_parse(gimp.gtkrc())

    import gimpprocbrowser
    dlg = gimpprocbrowser.dialog_new()

    gtk.main()

def query_pdb_browse():
    gimp.menu_register("python-fu-pdb-browse",
		       "<Toolbox>/Xtns/Languages/Python-Fu")

register(
    "python-fu-pdb-browse",
    "Browse the Procedural Database",
    "Pick a PDB proc, and read the information",
    "James Henstridge",
    "James Henstridge",
    "1997-1999",
    "_Procedure Browser",
    "",
    [],
    [],
    pdb_browse, on_query=query_pdb_browse)

main()
