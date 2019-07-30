#!/usr/bin/python3

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

import gi
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

import sys
import pyconsole
#import gimpshelf, gimpui, pyconsole

import gettext
_ = gettext.gettext
def N_(message): return message

PROC_NAME = 'python-fu-console'

RESPONSE_BROWSE, RESPONSE_CLEAR, RESPONSE_SAVE = range(3)

def run(name, n_params, params):
    Gimp.ui_init ("python-console.py", False)

    namespace = {'__builtins__': __builtins__,
                 '__name__': '__main__', '__doc__': None,
                 'cairo': gi.repository.cairo,
                 'Gdk': gi.repository.Gdk,
                 'Gegl': gi.repository.Gegl,
                 'Gimp': gi.repository.Gimp,
                 'Gio': gi.repository.Gio,
                 'Gtk': gi.repository.Gtk,
                 'GdkPixbuf': gi.repository.GdkPixbuf,
                 'GLib': gi.repository.GLib,
                 'GObject': gi.repository.GObject,
                 'Pango': gi.repository.Pango }

    class GimpConsole(pyconsole.Console):
        def __init__(self, quit_func=None):
            banner = ('GIMP %s Python Console\nPython %s\n' %
                      (Gimp.version(), sys.version))
            pyconsole.Console.__init__(self,
                                       locals=namespace, banner=banner,
                                       quit_func=quit_func)
        def _commit(self):
            pyconsole.Console._commit(self)
            Gimp.displays_flush()

    class ConsoleDialog(Gimp.Dialog):
        def __init__(self):
            Gimp.Dialog.__init__(self)
            self.set_property("help-id", PROC_NAME)
            Gtk.Window.set_title(self, _("Python Console"))
            Gtk.Window.set_role(self, PROC_NAME)
            Gtk.Dialog.add_button(self, "Save", Gtk.ResponseType.OK)
            Gtk.Dialog.add_button(self, "Clear", RESPONSE_CLEAR)
            Gtk.Dialog.add_button(self, _("_Browse..."), RESPONSE_BROWSE)
            Gtk.Dialog.add_button(self, "Close", Gtk.ResponseType.CLOSE)

            Gtk.Widget.set_name (self, PROC_NAME)
            Gimp.Dialog.set_alternative_button_order_from_array(self,
                                                                [ Gtk.ResponseType.CLOSE,
                                                                  RESPONSE_BROWSE,
                                                                  RESPONSE_CLEAR,
                                                                  Gtk.ResponseType.OK ])

            self.cons = GimpConsole(quit_func=lambda: Gtk.main_quit())

            self.style_set (None, None)

            self.connect('response', self.response)
            self.connect('style-set', self.style_set)

            self.browse_dlg = None
            self.save_dlg = None

            vbox = Gtk.VBox(homogeneous=False, spacing=12)
            vbox.set_border_width(12)
            contents_area = Gtk.Dialog.get_content_area(self)
            contents_area.pack_start(vbox, True, True, 0)

            scrl_win = Gtk.ScrolledWindow()
            scrl_win.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.ALWAYS)
            vbox.pack_start(scrl_win, True, True, 0)

            scrl_win.add(self.cons)

            width, height = self.cons.get_default_size()
            minreq, requisition = Gtk.Widget.get_preferred_size(scrl_win.get_vscrollbar())
            sb_width  = requisition.width
            sb_height = requisition.height

            # Account for scrollbar width and border width to ensure
            # the text view gets a width of 80 characters. We don't care
            # so much whether the height will be exactly 40 characters.
            Gtk.Window.set_default_size(self, width + sb_width + 2 * 12, height)

        def style_set(self, old_style, user_data):
            pass
            #style = Gtk.Widget.get_style (self)
            #self.cons.stdout_tag.set_property ("foreground", style.text[Gtk.StateType.NORMAL])
            #self.cons.stderr_tag.set_property ("foreground", style.text[Gtk.StateType.INSENSITIVE])

        def response(self, dialog, response_id):
            if response_id == RESPONSE_BROWSE:
                self.browse()
            elif response_id == RESPONSE_CLEAR:
                self.cons.banner = None
                self.cons.clear()
            elif response_id == Gtk.ResponseType.OK:
                self.save_dialog()
            else:
                Gtk.main_quit()

            self.cons.grab_focus()

        def browse_response(self, dlg, response_id):
            if response_id != Gtk.ResponseType.APPLY:
                Gtk.Widget.hide(dlg)
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
                dlg = Gimp.ProcBrowserDialog()
                Gtk.Window.set_title(dlg, _("Python Procedure Browser"))
                Gtk.Window.set_role(dlg, PROC_NAME)
                Gtk.Dialog.add_button(dlg, "Apply", Gtk.ResponseType.APPLY)
                Gtk.Dialog.add_button(dlg, "Close", Gtk.ResponseType.CLOSE)

                Gtk.Dialog.set_default_response(self, Gtk.ResponseType.OK)
                Gimp.Dialog.set_alternative_button_order_from_array(dlg,
                                                                    [ Gtk.ResponseType.CLOSE,
                                                                      Gtk.ResponseType.APPLY ])

                dlg.connect('response', self.browse_response)
                dlg.connect('row-activated',
                            lambda dlg: dlg.response(Gtk.ResponseType.APPLY))

                self.browse_dlg = dlg

            Gtk.Window.present(self.browse_dlg)

        def save_response(self, dlg, response_id):
            if response_id == Gtk.ResponseType.DELETE_EVENT:
                self.save_dlg = None
                return
            elif response_id == Gtk.ResponseType.OK:
                filename = dlg.get_filename()

                try:
                    logfile = open(filename, 'w')
                except IOError as e:
                    Gimp.message(_("Could not open '%s' for writing: %s") %
                                 (filename, e.strerror))
                    return

                buffer = self.cons.buffer

                start = buffer.get_start_iter()
                end = buffer.get_end_iter()

                log = buffer.get_text(start, end, False)

                try:
                    logfile.write(log)
                    logfile.close()
                except IOError as e:
                    Gimp.message(_("Could not write to '%s': %s") %
                                 (filename, e.strerror))
                    return

            Gtk.Widget.hide(dlg)

        def save_dialog(self):
            if not self.save_dlg:
                dlg = Gtk.FileChooserDialog()
                Gtk.Window.set_title(dlg, _("Save Python-Fu Console Output"))
                Gtk.Window.set_transient_for(dlg, self)
                Gtk.Dialog.add_button(dlg, "Cancel", Gtk.ResponseType.CANCEL)
                Gtk.Dialog.add_button(dlg, "Save", Gtk.ResponseType.OK)
                Gtk.Dialog.set_default_response(self, Gtk.ResponseType.OK)
                Gimp.Dialog.set_alternative_button_order_from_array(dlg,
                                                                    [ Gtk.ResponseType.OK,
                                                                      Gtk.ResponseType.CANCEL ])

                dlg.connect('response', self.save_response)

                self.save_dlg = dlg

            self.save_dlg.present()

        def run(self):
            Gtk.Widget.show_all(self)
            Gtk.main()

    ConsoleDialog().run()
    retval = [Gimp.param_from_status (Gimp.PDBStatusType.SUCCESS)]
    return len(retval), retval

def query():
    param = Gimp.ParamDef()
    param.type = Gimp.PDBArgType.INT32
    param.name = "run-mode"
    param.description = _("Run mode")

    Gimp.install_procedure(
        PROC_NAME,
        N_("Interactive GIMP Python interpreter"),
        "Type in commands and see results",
        "James Henstridge",
        "James Henstridge",
        "1997-1999",
        N_("_Console"),
        "",
        Gimp.PDBProcType.PLUGIN,
        [ param ],
        [])
    Gimp.plugin_menu_register(PROC_NAME, "<Image>/Filters/Languages/Python-Fu")
    Gimp.plugin_domain_register("gimp30-python", Gimp.locale_directory())

info = Gimp.PlugInInfo ()
info.set_callbacks (None, None, query, run)
Gimp.main_legacy (info, sys.argv)
