#!/usr/bin/env python2

#   Gimp-Python - allows the writing of Gimp plugins in Python.
#   Copyright (C) 1997  James Henstridge <james@daa.com.au>
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
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.

from gimpfu import *

t = gettext.translation('gimp20-python', gimp.locale_directory, fallback=True)
_ = t.ugettext

PROC_NAME = 'python-fu-console'

RESPONSE_BROWSE, RESPONSE_CLEAR, RESPONSE_SAVE = range(3)

def do_console():
    import pygtk
    pygtk.require('2.0')

    import sys, gobject, gtk, gimpenums, gimpshelf, gimpui, pyconsole
    gimpui.gimp_ui_init ()

    namespace = {'__builtins__': __builtins__,
                 '__name__': '__main__', '__doc__': None,
                 'gimp': gimp, 'pdb': gimp.pdb,
                 'shelf': gimpshelf.shelf}

    for s in gimpenums.__dict__.keys():
        if s[0] != '_':
            namespace[s] = getattr(gimpenums, s)

    class GimpConsole(pyconsole.Console):
        def __init__(self, quit_func=None):
            banner = ('GIMP %s Python Console\nPython %s\n' %
                      (gimp.pdb.gimp_version(), sys.version))
            pyconsole.Console.__init__(self,
                                       locals=namespace, banner=banner,
                                       quit_func=quit_func)
        def _commit(self):
            pyconsole.Console._commit(self)
            gimp.displays_flush()

    class ConsoleDialog(gimpui.Dialog):
        def __init__(self):
            gimpui.Dialog.__init__(self, title=_("Python Console"),
                                   role=PROC_NAME, help_id=PROC_NAME,
                                   buttons=(gtk.STOCK_SAVE,  RESPONSE_SAVE,
                                            gtk.STOCK_CLEAR, RESPONSE_CLEAR,
                                            _("_Browse..."), RESPONSE_BROWSE,
                                            gtk.STOCK_CLOSE, gtk.RESPONSE_CLOSE))

            self.set_name (PROC_NAME)
            self.set_alternative_button_order((gtk.RESPONSE_CLOSE,
                                               RESPONSE_BROWSE,
                                               RESPONSE_CLEAR,
                                               RESPONSE_SAVE))

            self.cons = GimpConsole(quit_func=lambda: gtk.main_quit())

            self.style_set (None, None)

            self.connect('response', self.response)
            self.connect('style-set', self.style_set)

            self.browse_dlg = None
            self.save_dlg = None

            vbox = gtk.VBox(False, 12)
            vbox.set_border_width(12)
            self.vbox.pack_start(vbox)

            scrl_win = gtk.ScrolledWindow()
            scrl_win.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_ALWAYS)
            vbox.pack_start(scrl_win)

            scrl_win.add(self.cons)

            width, height = self.cons.get_default_size()
            sb_width, sb_height = scrl_win.get_vscrollbar().size_request()

            # Account for scrollbar width and border width to ensure
            # the text view gets a width of 80 characters. We don't care
            # so much whether the height will be exactly 40 characters.
            self.set_default_size(width + sb_width + 2 * 12, height)

        def style_set(self, old_style, user_data):
            style = self.get_style ()
            self.cons.stdout_tag.set_property ("foreground", style.text[gtk.STATE_NORMAL])
            self.cons.stderr_tag.set_property ("foreground", style.text[gtk.STATE_INSENSITIVE])

        def response(self, dialog, response_id):
            if response_id == RESPONSE_BROWSE:
                self.browse()
            elif response_id == RESPONSE_CLEAR:
                self.cons.banner = None
                self.cons.clear()
            elif response_id == RESPONSE_SAVE:
                self.save_dialog()
            else:
                gtk.main_quit()

            self.cons.grab_focus()

        def browse_response(self, dlg, response_id):
            if response_id != gtk.RESPONSE_APPLY:
                dlg.hide()
                return

            proc_name = dlg.get_selected()

            if not proc_name:
                return

            proc = pdb[proc_name]

            cmd = ''

            if len(proc.return_vals) > 0:
                cmd = ', '.join(x[1].replace('-', '_')
                                for x in proc.return_vals) + ' = '

            cmd = cmd + 'pdb.%s' % proc.proc_name.replace('-', '_')

            if len(proc.params) > 0 and proc.params[0][1] == 'run-mode':
                params = proc.params[1:]
            else:
                params = proc.params

            cmd = cmd + '(%s)' % ', '.join(x[1].replace('-', '_')
                                           for x in params)

            buffer = self.cons.buffer

            lines = buffer.get_line_count()
            iter = buffer.get_iter_at_line_offset(lines - 1, 4)
            buffer.delete(iter, buffer.get_end_iter())
            buffer.place_cursor(buffer.get_end_iter())
            buffer.insert_at_cursor(cmd)

        def browse(self):
            if not self.browse_dlg:
                dlg = gimpui.ProcBrowserDialog(_("Python Procedure Browser"),
                                               role=PROC_NAME,
                                               buttons=(gtk.STOCK_APPLY,
                                                        gtk.RESPONSE_APPLY,
                                                        gtk.STOCK_CLOSE,
                                                        gtk.RESPONSE_CLOSE))

                dlg.set_default_response(gtk.RESPONSE_APPLY)
                dlg.set_alternative_button_order((gtk.RESPONSE_CLOSE,
                                                  gtk.RESPONSE_APPLY))

                dlg.connect('response', self.browse_response)
                dlg.connect('row-activated',
                            lambda dlg: dlg.response(gtk.RESPONSE_APPLY))

                self.browse_dlg = dlg

            self.browse_dlg.present()

        def save_response(self, dlg, response_id):
            if response_id == gtk.RESPONSE_DELETE_EVENT:
                self.save_dlg = None
                return
            elif response_id == gtk.RESPONSE_OK:
                filename = dlg.get_filename()

                try:
                    logfile = open(filename, 'w')
                except IOError, e:
                    gimp.message(_("Could not open '%s' for writing: %s") %
                                 (filename, e.strerror))
                    return

                buffer = self.cons.buffer

                start = buffer.get_start_iter()
                end = buffer.get_end_iter()

                log = buffer.get_text(start, end, False)

                try:
                    logfile.write(log)
                    logfile.close()
                except IOError, e:
                    gimp.message(_("Could not write to '%s': %s") %
                                 (filename, e.strerror))
                    return

            dlg.hide()

        def save_dialog(self):
            if not self.save_dlg:
                dlg = gtk.FileChooserDialog(_("Save Python-Fu Console Output"),
                                            parent=self,
                                            action=gtk.FILE_CHOOSER_ACTION_SAVE,
                                            buttons=(gtk.STOCK_CANCEL,
                                                     gtk.RESPONSE_CANCEL,
                                                     gtk.STOCK_SAVE,
                                                     gtk.RESPONSE_OK))

                dlg.set_default_response(gtk.RESPONSE_OK)
                dlg.set_alternative_button_order((gtk.RESPONSE_OK,
                                                  gtk.RESPONSE_CANCEL))

                dlg.connect('response', self.save_response)

                self.save_dlg = dlg

            self.save_dlg.present()

        def run(self):
            self.show_all()
            gtk.main()

    ConsoleDialog().run()

register(
    PROC_NAME,
    N_("Interactive GIMP Python interpreter"),
    "Type in commands and see results",
    "James Henstridge",
    "James Henstridge",
    "1997-1999",
    N_("_Console"),
    "",
    [],
    [],
    do_console,
    menu="<Image>/Filters/Languages/Python-Fu",
    domain=("gimp20-python", gimp.locale_directory))

main()
