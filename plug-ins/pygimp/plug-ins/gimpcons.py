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

gettext.install("gimp20-python", gimp.locale_directory, unicode=1)

PROC_NAME = 'python-fu-console'

RESPONSE_BROWSE, RESPONSE_CLEAR, RESPONSE_SAVE = range(3)

def console():
    import pygtk
    pygtk.require('2.0')

    import sys, gobject, gtk, gimpenums, gimpshelf, gimpui

    namespace = {'__builtins__': __builtins__,
                 '__name__': '__main__', '__doc__': None,
                 'gimp': gimp, 'pdb': gimp.pdb,
                 'shelf': gimpshelf.shelf}

    for s in gimpenums.__dict__.keys():
        if s[0] != '_':
            namespace[s] = getattr(gimpenums, s)

    def response(dialog, response_id, cons):
        if response_id == RESPONSE_BROWSE:
            browse(cons)
        elif response_id == RESPONSE_CLEAR:
            cons.banner = None
            cons.clear()
        elif response_id == RESPONSE_SAVE:
            save_dialog(dialog, cons)
        else:
            gtk.main_quit()
        

    dialog = gimpui.Dialog(title=_("Python Console"), role=PROC_NAME,
                           help_id=PROC_NAME,
                           buttons=(gtk.STOCK_SAVE,  RESPONSE_SAVE,
                                    gtk.STOCK_CLEAR, RESPONSE_CLEAR,
                                    _("_Browse..."), RESPONSE_BROWSE,
                                    gtk.STOCK_CLOSE, gtk.RESPONSE_CLOSE))
    dialog.set_alternative_button_order((gtk.RESPONSE_CLOSE, RESPONSE_BROWSE,
                                         RESPONSE_CLEAR, RESPONSE_SAVE))

    banner = 'Gimp %s Python Console\nPython %s\n' % (gimp.pdb.gimp_version(),
                                                      sys.version)

    import pyconsole
    cons = pyconsole.Console(locals=namespace, banner=banner,
                             quit_func=lambda: gtk.main_quit())

    dialog.connect("response", response, cons)

    def browse_response(dlg, response_id, cons):
        if response_id != gtk.RESPONSE_APPLY:
            dlg.destroy()
            return

        proc_name = dlg.get_selected()

        if not proc_name:
            return

        proc = pdb[proc_name]
            
        cmd = ''

        if len(proc.return_vals) > 0:
            cmd = ', '.join([x[1].replace('-', '_') for x in proc.return_vals]) + ' = '

        cmd = cmd + "pdb.%s" % proc.proc_name.replace('-', '_')

        if len(proc.params) > 0 and proc.params[0][1] == 'run-mode':
            params = proc.params[1:]
        else:
            params = proc.params

        cmd = cmd + "(%s)" % ', '.join([x[1].replace('-', '_') for x in params])

        cons.buffer.insert_at_cursor(cmd)

    def browse(cons):
        dlg = gimpui.ProcBrowserDialog(_("Python Procedure Browser"), PROC_NAME,
                                       buttons=(gtk.STOCK_APPLY, gtk.RESPONSE_APPLY,
                                                gtk.STOCK_CLOSE, gtk.RESPONSE_CLOSE))
        dlg.set_default_response(gtk.RESPONSE_APPLY)
        dlg.set_alternative_button_order((gtk.RESPONSE_CLOSE, gtk.RESPONSE_APPLY))

        dlg.connect("response", browse_response, cons)
        dlg.connect("row-activated",
                    lambda dlg: dlg.response(gtk.RESPONSE_APPLY))

        dlg.show()

    def save_dialog(parent, cons):
        dlg = gtk.FileChooserDialog(title=_("Save Python-Fu Console Output"),
                                    parent=parent,
                                    action=gtk.FILE_CHOOSER_ACTION_SAVE,
                                    buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                             gtk.STOCK_SAVE,   gtk.RESPONSE_OK))
        dlg.set_default_response(gtk.RESPONSE_OK)
        dlg.set_alternative_button_order((gtk.RESPONSE_OK, gtk.RESPONSE_CANCEL))

        def save_response(dlg, response_id, cons):
            if response_id == gtk.RESPONSE_OK:
                filename = dlg.get_filename()

                try:
                    logfile = open(filename, 'w')
                except IOError, e:
                    gimp.message(_("Could not open '%s' for writing: %s") %
                                 (filename, e.strerror))
                    return

                start = cons.buffer.get_start_iter()
                end = cons.buffer.get_end_iter()

                log = cons.buffer.get_text(start, end, False)

                logfile.write(log)
                logfile.close()

            dlg.destroy()

        dlg.connect("response", save_response, cons)
        dlg.present()

    vbox = gtk.VBox(False, 12)
    vbox.set_border_width(12)
    dialog.vbox.pack_start(vbox)

    scrl_win = gtk.ScrolledWindow()
    scrl_win.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_ALWAYS)
    vbox.pack_start(scrl_win)

    scrl_win.add(cons)

    dialog.set_default_size(500, 500)
    dialog.show_all()

    # flush the displays every half second
    def timeout():
        gimp.displays_flush()
        return True

    gobject.timeout_add(500, timeout)
    gtk.main()

register(
    PROC_NAME,
    N_("Interactive Gimp-Python interpreter"),
    "Type in commands and see results",
    "James Henstridge",
    "James Henstridge",
    "1997-1999",
    N_("_Console"),
    "",
    [],
    [],
    console,
    menu="<Toolbox>/Xtns/Languages/Python-Fu",
    domain=("gimp20-python", gimp.locale_directory))

main()
