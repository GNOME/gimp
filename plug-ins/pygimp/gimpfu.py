#   Gimp-Python - allows the writing of GIMP plug-ins in Python.
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
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""Simple interface for writing GIMP plug-ins in Python.

Instead of worrying about all the user interaction, saving last used
values and everything, the gimpfu module can take care of it for you.
It provides a simple register() function that will register your
plug-in if needed, and cause your plug-in function to be called when
needed.

Gimpfu will also handle showing a user interface for editing plug-in
parameters if the plug-in is called interactively, and will also save
the last used parameters, so the RUN_WITH_LAST_VALUES run_type will
work correctly.  It will also make sure that the displays are flushed
on completion if the plug-in was run interactively.

When registering the plug-in, you do not need to worry about
specifying the run_type parameter.

A typical gimpfu plug-in would look like this:
  from gimpfu import *

  def plugin_func(image, drawable, args):
              # do what plugins do best
  register(
              "plugin_func",
              "blurb",
              "help message",
              "author",
              "copyright",
              "year",
              "My plug-in",
              "*",
              [
                  (PF_IMAGE, "image", "Input image"),
                  (PF_DRAWABLE, "drawable", "Input drawable"),
                  (PF_STRING, "arg", "The argument", "default-value")
              ],
              [],
              plugin_func, menu="<Image>/Somewhere")
  main()

The call to "from gimpfu import *" will import all the gimp constants
into the plug-in namespace, and also import the symbols gimp, pdb,
register and main.  This should be just about all any plug-in needs.

You can use any of the PF_* constants below as parameter types, and an
appropriate user interface element will be displayed when the plug-in
is run in interactive mode.  Note that the the PF_SPINNER and
PF_SLIDER types expect a fifth element in their description tuple -- a
3-tuple of the form (lower,upper,step), which defines the limits for
the slider or spinner.

If want to localize your plug-in, add an optional domain parameter to
the register call. It can be the name of the translation domain or a
tuple that consists of the translation domain and the directory where
the translations are installed.
"""

import string as _string
import math
import gimp
import gimpcolor
from gimpenums import *
pdb = gimp.pdb

import gettext
t = gettext.translation("gimp20-python", gimp.locale_directory, fallback=True)
_ = t.ugettext

class error(RuntimeError): pass
class CancelError(RuntimeError): pass

PF_INT8        = PDB_INT8
PF_INT16       = PDB_INT16
PF_INT32       = PDB_INT32
PF_INT         = PF_INT32
PF_FLOAT       = PDB_FLOAT
PF_STRING      = PDB_STRING
PF_VALUE       = PF_STRING
#PF_INT8ARRAY   = PDB_INT8ARRAY
#PF_INT16ARRAY  = PDB_INT16ARRAY
#PF_INT32ARRAY  = PDB_INT32ARRAY
#PF_INTARRAY    = PF_INT32ARRAY
#PF_FLOATARRAY  = PDB_FLOATARRAY
#PF_STRINGARRAY = PDB_STRINGARRAY
PF_COLOR       = PDB_COLOR
PF_COLOUR      = PF_COLOR
PF_REGION      = PDB_REGION
PF_DISPLAY     = PDB_DISPLAY
PF_IMAGE       = PDB_IMAGE
PF_LAYER       = PDB_LAYER
PF_CHANNEL     = PDB_CHANNEL
PF_DRAWABLE    = PDB_DRAWABLE
PF_VECTORS     = PDB_VECTORS
#PF_SELECTION   = PDB_SELECTION
#PF_BOUNDARY    = PDB_BOUNDARY
#PF_PATH        = PDB_PATH
#PF_STATUS      = PDB_STATUS

PF_TOGGLE      = 1000
PF_BOOL        = PF_TOGGLE
PF_SLIDER      = 1001
PF_SPINNER     = 1002
PF_ADJUSTMENT  = PF_SPINNER

PF_FONT        = 1003
PF_FILE        = 1004
PF_BRUSH       = 1005
PF_PATTERN     = 1006
PF_GRADIENT    = 1007
PF_RADIO       = 1008
PF_TEXT        = 1009
PF_PALETTE     = 1010
PF_FILENAME    = 1011
PF_DIRNAME     = 1012
PF_OPTION      = 1013

_type_mapping = {
    PF_INT8        : PDB_INT8,
    PF_INT16       : PDB_INT16,
    PF_INT32       : PDB_INT32,
    PF_FLOAT       : PDB_FLOAT,
    PF_STRING      : PDB_STRING,
    #PF_INT8ARRAY   : PDB_INT8ARRAY,
    #PF_INT16ARRAY  : PDB_INT16ARRAY,
    #PF_INT32ARRAY  : PDB_INT32ARRAY,
    #PF_FLOATARRAY  : PDB_FLOATARRAY,
    #PF_STRINGARRAY : PDB_STRINGARRAY,
    PF_COLOR       : PDB_COLOR,
    PF_REGION      : PDB_REGION,
    PF_DISPLAY     : PDB_DISPLAY,
    PF_IMAGE       : PDB_IMAGE,
    PF_LAYER       : PDB_LAYER,
    PF_CHANNEL     : PDB_CHANNEL,
    PF_DRAWABLE    : PDB_DRAWABLE,
    PF_VECTORS     : PDB_VECTORS,

    PF_TOGGLE      : PDB_INT32,
    PF_SLIDER      : PDB_FLOAT,
    PF_SPINNER     : PDB_INT32,

    PF_FONT        : PDB_STRING,
    PF_FILE        : PDB_STRING,
    PF_BRUSH       : PDB_STRING,
    PF_PATTERN     : PDB_STRING,
    PF_GRADIENT    : PDB_STRING,
    PF_RADIO       : PDB_STRING,
    PF_TEXT        : PDB_STRING,
    PF_PALETTE     : PDB_STRING,
    PF_FILENAME    : PDB_STRING,
    PF_DIRNAME     : PDB_STRING,
    PF_OPTION      : PDB_INT32,
}

_obj_mapping = {
    PF_INT8        : int,
    PF_INT16       : int,
    PF_INT32       : int,
    PF_FLOAT       : float,
    PF_STRING      : str,
    #PF_INT8ARRAY   : list,
    #PF_INT16ARRAY  : list,
    #PF_INT32ARRAY  : list,
    #PF_FLOATARRAY  : list,
    #PF_STRINGARRAY : list,
    PF_COLOR       : gimpcolor.RGB,
    PF_REGION      : int,
    PF_DISPLAY     : gimp.Display,
    PF_IMAGE       : gimp.Image,
    PF_LAYER       : gimp.Layer,
    PF_CHANNEL     : gimp.Channel,
    PF_DRAWABLE    : gimp.Drawable,
    PF_VECTORS     : gimp.Vectors,

    PF_TOGGLE      : bool,
    PF_SLIDER      : float,
    PF_SPINNER     : int,

    PF_FONT        : str,
    PF_FILE        : str,
    PF_BRUSH       : str,
    PF_PATTERN     : str,
    PF_GRADIENT    : str,
    PF_RADIO       : str,
    PF_TEXT        : str,
    PF_PALETTE     : str,
    PF_FILENAME    : str,
    PF_DIRNAME     : str,
    PF_OPTION      : int,
}

_registered_plugins_ = {}

def register(proc_name, blurb, help, author, copyright, date, label,
             imagetypes, params, results, function,
             menu=None, domain=None, on_query=None, on_run=None):
    """This is called to register a new plug-in."""

    # First perform some sanity checks on the data
    def letterCheck(str):
        allowed = _string.letters + _string.digits + "_" + "-"
        for ch in str:
            if not ch in allowed:
		return 0
        else:
            return 1

    if not letterCheck(proc_name):
        raise error, "procedure name contains illegal characters"

    for ent in params:
        if len(ent) < 4:
            raise error, ("parameter definition must contain at least 4 "
                          "elements (%s given: %s)" % (len(ent), ent))

        if type(ent[0]) != int:
            raise error, "parameter types must be integers"

        if not letterCheck(ent[1]):
            raise error, "parameter name contains illegal characters"

    for ent in results:
        if len(ent) < 3:
            raise error, ("result definition must contain at least 3 elements "
                          "(%s given: %s)" % (len(ent), ent))

        if type(ent[0]) != type(42):
            raise error, "result types must be integers"

        if not letterCheck(ent[1]):
            raise error, "result name contains illegal characters"

    plugin_type = PLUGIN

    if (not proc_name.startswith("python-") and
        not proc_name.startswith("python_") and
        not proc_name.startswith("extension-") and
        not proc_name.startswith("extension_") and
        not proc_name.startswith("plug-in-") and
        not proc_name.startswith("plug_in_") and
        not proc_name.startswith("file-") and
        not proc_name.startswith("file_")):
           proc_name = "python-fu-" + proc_name

    # if menu is not given, derive it from label
    need_compat_params = False
    if menu is None and label:
        fields = label.split("/")
        if fields:
            label = fields.pop()
            menu = "/".join(fields)
            need_compat_params = True

            import warnings
            message = ("%s: passing the full menu path for the menu label is "
                       "deprecated, use the 'menu' parameter instead"
                       % (proc_name))
            warnings.warn(message, DeprecationWarning, 3)

        if need_compat_params and plugin_type == PLUGIN:
            file_params = [(PDB_STRING, "filename", "The name of the file", ""),
                           (PDB_STRING, "raw-filename", "The name of the file", "")]

            if menu is None:
                pass
            elif menu.startswith("<Load>"):
                params[0:0] = file_params
            elif menu.startswith("<Image>") or menu.startswith("<Save>"):
                params.insert(0, (PDB_IMAGE, "image", "Input image", None))
                params.insert(1, (PDB_DRAWABLE, "drawable", "Input drawable", None))
                if menu.startswith("<Save>"):
                    params[2:2] = file_params

    _registered_plugins_[proc_name] = (blurb, help, author, copyright,
                                       date, label, imagetypes,
                                       plugin_type, params, results,
                                       function, menu, domain,
                                       on_query, on_run)

def _query():
    for plugin in _registered_plugins_.keys():
        (blurb, help, author, copyright, date,
         label, imagetypes, plugin_type,
         params, results, function, menu, domain,
         on_query, on_run) = _registered_plugins_[plugin]

        def make_params(params):
            return [(_type_mapping[x[0]],
                     x[1],
                     _string.replace(x[2], "_", "")) for x in params]

        params = make_params(params)
        # add the run mode argument ...
        params.insert(0, (PDB_INT32, "run-mode",
                                     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"))

        results = make_params(results)

        if domain:
            try:
                (domain, locale_dir) = domain
                gimp.domain_register(domain, locale_dir)
            except ValueError:
                gimp.domain_register(domain)

        gimp.install_procedure(plugin, blurb, help, author, copyright,
                               date, label, imagetypes, plugin_type,
                               params, results)

        if menu:
            gimp.menu_register(plugin, menu)
        if on_query:
            on_query()

def _get_defaults(proc_name):
    import gimpshelf

    (blurb, help, author, copyright, date,
     label, imagetypes, plugin_type,
     params, results, function, menu, domain,
     on_query, on_run) = _registered_plugins_[proc_name]

    key = "python-fu-save--" + proc_name

    if gimpshelf.shelf.has_key(key):
        return gimpshelf.shelf[key]
    else:
        # return the default values
        return [x[3] for x in params]

def _set_defaults(proc_name, defaults):
    import gimpshelf

    key = "python-fu-save--" + proc_name
    gimpshelf.shelf[key] = defaults

def _interact(proc_name, start_params):
    (blurb, help, author, copyright, date,
     label, imagetypes, plugin_type,
     params, results, function, menu, domain,
     on_query, on_run) = _registered_plugins_[proc_name]

    def run_script(run_params):
        params = start_params + tuple(run_params)
        _set_defaults(proc_name, params)
        return apply(function, params)

    params = params[len(start_params):]

    # short circuit for no parameters ...
    if len(params) == 0:
         return run_script([])

    import pygtk
    pygtk.require('2.0')

    import gimpui
    import gtk
#    import pango

    defaults = _get_defaults(proc_name)
    defaults = defaults[len(start_params):]

    class EntryValueError(Exception):
        pass

    def warning_dialog(parent, primary, secondary=None):
        dlg = gtk.MessageDialog(parent, gtk.DIALOG_DESTROY_WITH_PARENT,
                                        gtk.MESSAGE_WARNING, gtk.BUTTONS_CLOSE,
                                        primary)
        if secondary:
            dlg.format_secondary_text(secondary)
        dlg.run()
        dlg.destroy()

    def error_dialog(parent, proc_name):
        import sys, traceback

        exc_str = exc_only_str = _("Missing exception information")

        try:
            etype, value, tb = sys.exc_info()
            exc_str = "".join(traceback.format_exception(etype, value, tb))
            exc_only_str = "".join(traceback.format_exception_only(etype, value))
        finally:
            etype = value = tb = None

        title = _("An error occurred running %s") % proc_name
        dlg = gtk.MessageDialog(parent, gtk.DIALOG_DESTROY_WITH_PARENT,
                                        gtk.MESSAGE_ERROR, gtk.BUTTONS_CLOSE,
                                        title)
        dlg.format_secondary_text(exc_only_str)

        alignment = gtk.Alignment(0.0, 0.0, 1.0, 1.0)
        alignment.set_padding(0, 0, 12, 12)
        dlg.vbox.pack_start(alignment)
        alignment.show()

        expander = gtk.Expander(_("_More Information"));
        expander.set_use_underline(True)
        expander.set_spacing(6)
        alignment.add(expander)
        expander.show()

        scrolled = gtk.ScrolledWindow()
        scrolled.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        scrolled.set_size_request(-1, 200)
        expander.add(scrolled)
        scrolled.show()


        label = gtk.Label(exc_str)
        label.set_alignment(0.0, 0.0)
        label.set_padding(6, 6)
        label.set_selectable(True)
        scrolled.add_with_viewport(label)
        label.show()

        def response(widget, id):
            widget.destroy()

        dlg.connect("response", response)
        dlg.set_resizable(True)
        dlg.show()

    # define a mapping of param types to edit objects ...
    class StringEntry(gtk.Entry):
        def __init__(self, default=""):
            gtk.Entry.__init__(self)
            self.set_text(str(default))

        def get_value(self):
            return self.get_text()

    class TextEntry(gtk.ScrolledWindow):
        def __init__ (self, default=""):
            gtk.ScrolledWindow.__init__(self)
            self.set_shadow_type(gtk.SHADOW_IN)

            self.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
            self.set_size_request(100, -1)

            self.view = gtk.TextView()
            self.add(self.view)
            self.view.show()

            self.buffer = self.view.get_buffer()

            self.set_value(str(default))

        def set_value(self, text):
            self.buffer.set_text(text)

        def get_value(self):
            return self.buffer.get_text(self.buffer.get_start_iter(),
                                        self.buffer.get_end_iter())

    class IntEntry(StringEntry):
        def get_value(self):
            try:
                return int(self.get_text())
            except ValueError, e:
                raise EntryValueError, e.args

    class FloatEntry(StringEntry):
            def get_value(self):
                try:
                    return float(self.get_text())
                except ValueError, e:
                    raise EntryValueError, e.args

#    class ArrayEntry(StringEntry):
#            def get_value(self):
#                return eval(self.get_text(), {}, {})


    def precision(step):
        # calculate a reasonable precision from a given step size
        if math.fabs(step) >= 1.0 or step == 0.0:
            digits = 0
        else:
            digits = abs(math.floor(math.log10(math.fabs(step))));
        if digits > 20:
            digits = 20
        return int(digits)

    class SliderEntry(gtk.HScale):
        # bounds is (upper, lower, step)
        def __init__(self, default=0, bounds=(0, 100, 5)):
            step = bounds[2]
            self.adj = gtk.Adjustment(default, bounds[0], bounds[1],
                                      step, 10 * step, 0)
            gtk.HScale.__init__(self, self.adj)
            self.set_digits(precision(step))

        def get_value(self):
            return self.adj.value

    class SpinnerEntry(gtk.SpinButton):
        # bounds is (upper, lower, step)
        def __init__(self, default=0, bounds=(0, 100, 5)):
            step = bounds[2]
            self.adj = gtk.Adjustment(default, bounds[0], bounds[1],
                                      step, 10 * step, 0)
            gtk.SpinButton.__init__(self, self.adj, step, precision(step))

    class ToggleEntry(gtk.ToggleButton):
        def __init__(self, default=0):
            gtk.ToggleButton.__init__(self)

            self.label = gtk.Label(_("No"))
            self.add(self.label)
            self.label.show()

            self.connect("toggled", self.changed)

            self.set_active(default)

        def changed(self, tog):
            if tog.get_active():
                self.label.set_text(_("Yes"))
            else:
                self.label.set_text(_("No"))

        def get_value(self):
            return self.get_active()

    class RadioEntry(gtk.VBox):
        def __init__(self, default=0, items=((_("Yes"), 1), (_("No"), 0))):
            gtk.VBox.__init__(self, homogeneous=False, spacing=2)

            button = None

            for (label, value) in items:
                button = gtk.RadioButton(button, label)
                self.pack_start(button)
                button.show()

                button.connect("toggled", self.changed, value)

                if value == default:
                    button.set_active(True)
                    self.active_value = value

        def changed(self, radio, value):
            if radio.get_active():
                self.active_value = value

        def get_value(self):
            return self.active_value

    class ComboEntry(gtk.ComboBox):
        def __init__(self, default=0, items=()):
            store = gtk.ListStore(str)
            for item in items:
                store.append([item])

            gtk.ComboBox.__init__(self, model=store)

            cell = gtk.CellRendererText()
            self.pack_start(cell)
            self.set_attributes(cell, text=0)

            self.set_active(default)

        def get_value(self):
            return self.get_active()

    def FileSelector(default=""):
        # FIXME: should this be os.path.separator?  If not, perhaps explain why?
        if default and default.endswith("/"):
            selector = DirnameSelector
            if default == "/": default = ""
        else:
            selector = FilenameSelector
        return selector(default)

    class FilenameSelector(gtk.FileChooserButton):
        def __init__(self, default="", save_mode=False):
            gtk.FileChooserButton.__init__(self,
                                           _("Python-Fu File Selection"))
            self.set_action(gtk.FILE_CHOOSER_ACTION_OPEN)
            if default:
                self.set_filename(default)

        def get_value(self):
            return self.get_filename()

    class DirnameSelector(gtk.FileChooserButton):
        def __init__(self, default=""):
            gtk.FileChooserButton.__init__(self,
                                           _("Python-Fu Folder Selection"))
            self.set_action(gtk.FILE_CHOOSER_ACTION_SELECT_FOLDER)
            if default:
                self.set_filename(default)

        def get_value(self):
            return self.get_filename()

    _edit_mapping = {
            PF_INT8        : IntEntry,
            PF_INT16       : IntEntry,
            PF_INT32       : IntEntry,
            PF_FLOAT       : FloatEntry,
            PF_STRING      : StringEntry,
            #PF_INT8ARRAY   : ArrayEntry,
            #PF_INT16ARRAY  : ArrayEntry,
            #PF_INT32ARRAY  : ArrayEntry,
            #PF_FLOATARRAY  : ArrayEntry,
            #PF_STRINGARRAY : ArrayEntry,
            PF_COLOR       : gimpui.ColorSelector,
            PF_REGION      : IntEntry,  # should handle differently ...
            PF_IMAGE       : gimpui.ImageSelector,
            PF_LAYER       : gimpui.LayerSelector,
            PF_CHANNEL     : gimpui.ChannelSelector,
            PF_DRAWABLE    : gimpui.DrawableSelector,
            PF_VECTORS     : gimpui.VectorsSelector,

            PF_TOGGLE      : ToggleEntry,
            PF_SLIDER      : SliderEntry,
            PF_SPINNER     : SpinnerEntry,
            PF_RADIO       : RadioEntry,
            PF_OPTION      : ComboEntry,

            PF_FONT        : gimpui.FontSelector,
            PF_FILE        : FileSelector,
            PF_FILENAME    : FilenameSelector,
            PF_DIRNAME     : DirnameSelector,
            PF_BRUSH       : gimpui.BrushSelector,
            PF_PATTERN     : gimpui.PatternSelector,
            PF_GRADIENT    : gimpui.GradientSelector,
            PF_PALETTE     : gimpui.PaletteSelector,
            PF_TEXT        : TextEntry
    }

    if on_run:
        on_run()

    dialog = gimpui.Dialog(proc_name, "python-fu", None, 0, None, proc_name,
                           (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                            gtk.STOCK_OK, gtk.RESPONSE_OK))

    dialog.set_alternative_button_order((gtk.RESPONSE_OK, gtk.RESPONSE_CANCEL))

    dialog.set_transient()

    vbox = gtk.VBox(False, 12)
    vbox.set_border_width(12)
    dialog.vbox.pack_start(vbox)
    vbox.show()

    if blurb:
        if domain:
            try:
                (domain, locale_dir) = domain
                trans = gettext.translation(domain, locale_dir, fallback=True)
            except ValueError:
                trans = gettext.translation(domain, fallback=True)
            blurb = trans.ugettext(blurb)
        box = gimpui.HintBox(blurb)
        vbox.pack_start(box, expand=False)
        box.show()

    table = gtk.Table(len(params), 2, False)
    table.set_row_spacings(6)
    table.set_col_spacings(6)
    vbox.pack_start(table, expand=False)
    table.show()

    def response(dlg, id):
        if id == gtk.RESPONSE_OK:
            dlg.set_response_sensitive(gtk.RESPONSE_OK, False)
            dlg.set_response_sensitive(gtk.RESPONSE_CANCEL, False)

            params = []

            try:
                for wid in edit_wids:
                    params.append(wid.get_value())
            except EntryValueError:
                warning_dialog(dialog, _("Invalid input for '%s'") % wid.desc)
            else:
                try:
                    dialog.res = run_script(params)
                except Exception:
                    dlg.set_response_sensitive(gtk.RESPONSE_CANCEL, True)
                    error_dialog(dialog, proc_name)
                    raise

        gtk.main_quit()

    dialog.connect("response", response)

    edit_wids = []
    for i in range(len(params)):
        pf_type = params[i][0]
        name = params[i][1]
        desc = params[i][2]
        def_val = defaults[i]

        label = gtk.Label(desc)
        label.set_use_underline(True)
        label.set_alignment(0.0, 0.5)
        table.attach(label, 1, 2, i, i+1, xoptions=gtk.FILL)
        label.show()

        if pf_type in (PF_SPINNER, PF_SLIDER, PF_RADIO, PF_OPTION):
            wid = _edit_mapping[pf_type](def_val, params[i][4])
        else:
            wid = _edit_mapping[pf_type](def_val)

        label.set_mnemonic_widget(wid)

        table.attach(wid, 2,3, i,i+1, yoptions=0)

        if pf_type != PF_TEXT:
            wid.set_tooltip_text(desc)
        else:
            # Attach tip to TextView, not to ScrolledWindow
            wid.view.set_tooltip_text(desc)
        wid.show()

        wid.desc = desc
        edit_wids.append(wid)

    progress_vbox = gtk.VBox(False, 6)
    vbox.pack_end(progress_vbox, expand=False)
    progress_vbox.show()

    progress = gimpui.ProgressBar()
    progress_vbox.pack_start(progress)
    progress.show()

#    progress_label = gtk.Label()
#    progress_label.set_alignment(0.0, 0.5)
#    progress_label.set_ellipsize(pango.ELLIPSIZE_MIDDLE)

#    attrs = pango.AttrList()
#    attrs.insert(pango.AttrStyle(pango.STYLE_ITALIC, 0, -1))
#    progress_label.set_attributes(attrs)

#    progress_vbox.pack_start(progress_label)
#    progress_label.show()

    dialog.show()

    gtk.main()

    if hasattr(dialog, "res"):
        res = dialog.res
        dialog.destroy()
        return res
    else:
        dialog.destroy()
        raise CancelError

def _run(proc_name, params):
    run_mode = params[0]
    func = _registered_plugins_[proc_name][10]

    if run_mode == RUN_NONINTERACTIVE:
        return apply(func, params[1:])

    script_params = _registered_plugins_[proc_name][8]

    min_args = 0
    if len(params) > 1:
        for i in range(1, len(params)):
            param_type = _obj_mapping[script_params[i - 1][0]]
            if not isinstance(params[i], param_type):
                break

        min_args = i

    if len(script_params) > min_args:
        start_params = params[:min_args + 1]

        if run_mode == RUN_WITH_LAST_VALS:
            default_params = _get_defaults(proc_name)
            params = start_params + default_params[min_args:]
        else:
            params = start_params
    else:
       run_mode = RUN_NONINTERACTIVE

    if run_mode == RUN_INTERACTIVE:
        try:
            res = _interact(proc_name, params[1:])
        except CancelError:
            return
    else:
        res = apply(func, params[1:])

    gimp.displays_flush()

    return res

def main():
    """This should be called after registering the plug-in."""
    gimp.main(None, None, _query, _run)

def fail(msg):
    """Display an error message and quit"""
    gimp.message(msg)
    raise error, msg

def N_(message):
    return message
