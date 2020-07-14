
'''

!!!
This all should go away when Gimp support for auto plugin GUI lands in Gimp 3.
Not well known that such support is on the roadmap.
!!!

'''



import gi

gi.require_version("Gimp", "3.0")
from gi.repository import Gimp  # only for locale?

gi.require_version("Gtk", "3.0")
from gi.repository import Gtk


import gettext
t = gettext.translation("gimp30-python", Gimp.locale_directory, fallback=True)
_ = t.gettext







class StringEntry(Gtk.Entry):
    def __init__(self, default=""):
        Gtk.Entry.__init__(self)
        self.set_text(str(default))
        self.set_activates_default(True)

    def get_value(self):
        return self.get_text()
'''
class TextEntry(Gtk.ScrolledWindow):
    def __init__ (self, default=""):
        Gtk.ScrolledWindow.__init__(self)
        self.set_shadow_type(Gtk.SHADOW_IN)

        self.set_policy(Gtk.POLICY_AUTOMATIC, Gtk.POLICY_AUTOMATIC)
        self.set_size_request(100, -1)

        self.view = Gtk.TextView()
        self.add(self.view)
        self.view.show()

        self.buffer = self.view.get_buffer()

        self.set_value(str(default))

    def set_value(self, text):
        self.buffer.set_text(text)

    def get_value(self):
        return self.buffer.get_text(self.buffer.get_start_iter(),
                                    self.buffer.get_end_iter())
'''
class IntEntry(StringEntry):
    def get_value(self):
        try:
            return int(self.get_text())
        except ValueError as e:
            raise EntryValueError(e.args)

class FloatEntry(StringEntry):
        def get_value(self):
            try:
                return float(self.get_text())
            except ValueError as e:
                raise EntryValueError(e.args)

'''
#    class ArrayEntry(StringEntry):
#            def get_value(self):
#                return eval(self.get_text(), {}, {})


def precision(step):
    import math

    # calculate a reasonable precision from a given step size
    if math.fabs(step) >= 1.0 or step == 0.0:
        digits = 0
    else:
        digits = abs(math.floor(math.log10(math.fabs(step))));
    if digits > 20:
        digits = 20
    return int(digits)

class SliderEntry(Gtk.HScale):
    # bounds is (upper, lower, step)
    def __init__(self, default=0, bounds=(0, 100, 5)):
        step = bounds[2]
        self.adj = Gtk.Adjustment(default, bounds[0], bounds[1],
                                  step, 10 * step, 0)
        Gtk.HScale.__init__(self, self.adj)
        self.set_digits(precision(step))

    def get_value(self):
        return self.adj.value

class SpinnerEntry(Gtk.SpinButton):
    # bounds is (upper, lower, step)
    def __init__(self, default=0, bounds=(0, 100, 5)):
        step = bounds[2]
        self.adj = Gtk.Adjustment(default, bounds[0], bounds[1],
                                  step, 10 * step, 0)
        Gtk.SpinButton.__init__(self, self.adj, step, precision(step))

'''

class ToggleEntry(Gtk.ToggleButton):
    def __init__(self, default=0):
        Gtk.ToggleButton.__init__(self)

        self.label = Gtk.Label(_("No"))
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



class RadioEntry(Gtk.VBox):
    def __init__(self, default=0, items=((_("Yes"), 1), (_("No"), 0))):
        Gtk.VBox.__init__(self, homogeneous=False, spacing=2)

        # TODO this is not correct

        #group = Gtk.RadioButtonGroup()
        #group.show()
        # OLD Gtk API passed previous button instead of group
        button = Gtk.RadioButton(None)

        for (label, value) in items:
            #button = Gtk.RadioButton(group, label)
            button = Gtk.RadioButton(button, label)
            self.pack_start(button, expand=True, fill=True, padding=0)
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


'''
To handle author errors.
Just a label, not allowing interaction.
But has a gettable value.
Value is a constant passed at create time, called 'default'
'''
class OmittedEntry(Gtk.Label):
    def __init__(self, default=0):
        super().__init__("Omitted")
        self.value = default

    def get_value(self):
        return self.value


'''
class ComboEntry(Gtk.ComboBox):
    def __init__(self, default=0, items=()):
        store = Gtk.ListStore(str)
        for item in items:
            store.append([item])

        Gtk.ComboBox.__init__(self, model=store)

        cell = Gtk.CellRendererText()
        self.pack_start(cell)
        self.set_attributes(cell, text=0)

        self.set_active(default)

    def get_value(self):
        return self.get_active()

def FileSelector(default="", title=None):
    # FIXME: should this be os.path.separator?  If not, perhaps explain why?
    if default and default.endswith("/"):
        if default == "/": default = ""
        return DirnameSelector(default)
    else:
        return FilenameSelector(default, title=title, save_mode=False)

class FilenameSelector(Gtk.HBox):
    #gimpfu.FileChooserButton
    def __init__(self, default, save_mode=True, title=None):
        super(FilenameSelector, self).__init__()
        if not title:
            self.title = _("Python-Fu File Selection")
        else:
            self.title = title
        self.save_mode = save_mode
        box = self
        self.entry = Gtk.Entry()
        image = Gtk.Image()
        image.set_from_stock(Gtk.STOCK_FILE, Gtk.ICON_SIZE_BUTTON)
        self.button = Gtk.Button()
        self.button.set_image(image)
        box.pack_start(self.entry)
        box.pack_start(self.button, expand=False)
        self.button.connect("clicked", self.pick_file)
        if default:
            self.entry.set_text(default)

    def show(self):
        super(FilenameSelector, self).show()
        self.button.show()
        self.entry.show()

    def pick_file(self, widget):
        entry = self.entry
        dialog = Gtk.FileChooserDialog(
                     title=self.title,
                     action=(Gtk.FILE_CHOOSER_ACTION_SAVE
                                 if self.save_mode else
                             Gtk.FILE_CHOOSER_ACTION_OPEN),
                     buttons=(Gtk.STOCK_CANCEL,
                            Gtk.RESPONSE_CANCEL,
                            Gtk.STOCK_SAVE
                                 if self.save_mode else
                            Gtk.STOCK_OPEN,
                            Gtk.RESPONSE_OK)
                    )
        dialog.set_alternative_button_order ((Gtk.RESPONSE_OK, Gtk.RESPONSE_CANCEL))
        dialog.show_all()
        response = dialog.run()
        if response == Gtk.RESPONSE_OK:
            entry.set_text(dialog.get_filename())
        dialog.destroy()

    def get_value(self):
        return self.entry.get_text()


class DirnameSelector(Gtk.FileChooserButton):
    def __init__(self, default=""):
        Gtk.FileChooserButton.__init__(self,
                                       _("Python-Fu Folder Selection"))
        self.set_action(Gtk.FILE_CHOOSER_ACTION_SELECT_FOLDER)
        if default:
            self.set_filename(default)

    def get_value(self):
        return self.get_filename()
'''
