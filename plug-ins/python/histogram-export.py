#!/usr/bin/env python3
#coding: utf-8

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

"""
Exports the image histogram to a text file,
so that it can be used by other programs
and loaded into spreadsheets.

The resulting file is a CSV file (Comma Separated
Values), which can be imported
directly in most spreadsheet programs.

The first two columns are the bucket boundaries,
followed by the selected columns. The histogram
refers to the selected image area, and
can use either Sample Average data or data
from the current drawable only.;

The output is in "weighted pixels" - meaning
all fully transparent pixels are not counted.

Check the gimp-histogram call
"""

import csv
import math
import sys

import gi
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp
gi.require_version('GimpUi', '3.0')
from gi.repository import GimpUi
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

def N_(message): return message
def _(message): return GLib.dgettext(None, message)

class StringEnum:
    """
    Helper class for when you want to use strings as keys of an enum. The values would be
    user facing strings that might undergo translation.

    The constructor accepts an even amount of arguments. Each pair of arguments
    is a key/value pair.
    """
    def __init__(self, *args):
        self.keys = []
        self.values = []

        for i in range(len(args)//2):
            self.keys.append(args[i*2])
            self.values.append(args[i*2+1])

    def get_tree_model(self):
        """ Get a tree model that can be used in GTK widgets. """
        tree_model = Gtk.ListStore(GObject.TYPE_STRING, GObject.TYPE_STRING)
        for i in range(len(self.keys)):
            tree_model.append([self.keys[i], self.values[i]])
        return tree_model

    def __getattr__(self, name):
        """ Implements access to the key. For example, if you provided a key "red", then you could access it by
            referring to
               my_enum.red
            It may seem silly as "my_enum.red" is longer to write then just "red",
            but this provides verification that the key is indeed inside enum. """
        key = name.replace("_", " ")
        if key in self.keys:
            return key
        raise AttributeError("No such key string " + key)


output_format_enum = StringEnum(
    "pixel count", _("Pixel count"),
    "normalized", _("Normalized"),
    "percent", _("Percent")
)


def histogram_export(procedure, img, layers, gio_file,
                     bucket_size, sample_average, output_format,
                     progress_bar):
    layers = img.get_selected_layers()
    layer = layers[0]
    if sample_average:
        new_img = img.duplicate()
        layer = new_img.merge_visible_layers(Gimp.MergeType.CLIP_TO_IMAGE)

    channels_txt = ["Value"]
    channels_gimp = [Gimp.HistogramChannel.VALUE]
    if layer.is_rgb():
        channels_txt += ["Red", "Green", "Blue", "Luminance"]
        channels_gimp += [Gimp.HistogramChannel.RED, Gimp.HistogramChannel.GREEN, Gimp.HistogramChannel.BLUE,
                          Gimp.HistogramChannel.LUMINANCE]
    if layer.has_alpha():
        channels_txt += ["Alpha"]
        channels_gimp += [Gimp.HistogramChannel.ALPHA]

    try:
        with open(gio_file.get_path(), "wt") as hfile:
            writer = csv.writer(hfile)

            # Write headers:
            writer.writerow(["Range start"] + channels_txt)

            max_index = 1.0/bucket_size if bucket_size > 0 else 1
            i = 0
            progress_bar_int_percent = 0
            while True:
                start_range = i * bucket_size
                i += 1
                if start_range >= 1.0:
                    break

                row = [start_range]
                for channel in channels_gimp:
                    result = Gimp.get_pdb().run_procedure('gimp-drawable-histogram',
                                                          [ GObject.Value(Gimp.Drawable, layer),
                                                            GObject.Value(Gimp.HistogramChannel, channel),
                                                            GObject.Value(GObject.TYPE_DOUBLE,
                                                                          float(start_range)),
                                                            GObject.Value(GObject.TYPE_DOUBLE,
                                                                          float(min(start_range + bucket_size, 1.0))) ])

                    if output_format == output_format_enum.pixel_count:
                        count = int(result.index(5))
                    else:
                        pixels = result.index(4)
                        count = (result.index(5) / pixels) if pixels else 0
                        if output_format == output_format_enum.percent:
                            count = "%.2f%%" % (count * 100)
                    row.append(str(count))
                writer.writerow(row)

                # Update progress bar
                if progress_bar:
                    fraction = i / max_index
                    # Only update the progress bar if it changed at least 1% .
                    new_percent = math.floor(fraction * 100)
                    if new_percent != progress_bar_int_percent:
                        progress_bar_int_percent = new_percent
                        progress_bar.set_fraction(fraction)
                        # Make sure the progress bar gets drawn on screen.
                        while Gtk.events_pending():
                            Gtk.main_iteration()
    except IsADirectoryError:
        return procedure.new_return_values(Gimp.PDBStatusType.EXECUTION_ERROR,
                                           GLib.Error(_("File is either a directory or file name is empty.")))
    except FileNotFoundError:
        return procedure.new_return_values(Gimp.PDBStatusType.EXECUTION_ERROR,
                                           GLib.Error(_("Directory not found.")))
    except PermissionError:
        return procedure.new_return_values(Gimp.PDBStatusType.EXECUTION_ERROR,
                                           GLib.Error("You do not have permissions to write that file."))

    if sample_average:
        new_img.delete()

    return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())


def run(procedure, run_mode, image, n_layers, layers, args, data):
    gio_file = args.index(0)
    bucket_size = args.index(1)
    sample_average = args.index(2)
    output_format = args.index(3)

    progress_bar = None
    config = None

    if run_mode == Gimp.RunMode.INTERACTIVE:

        config = procedure.create_config()

        # Set properties from arguments. These properties will be changed by the UI.
        #config.set_property("file", gio_file)
        #config.set_property("bucket_size", bucket_size)
        #config.set_property("sample_average", sample_average)
        #config.set_property("output_format", output_format)
        config.begin_run(image, run_mode, args)

        GimpUi.init("histogram-export.py")
        use_header_bar = Gtk.Settings.get_default().get_property("gtk-dialogs-use-header")
        dialog = GimpUi.Dialog(use_header_bar=use_header_bar,
                             title=_("Histogram Export..."))
        dialog.add_button(_("_Cancel"), Gtk.ResponseType.CANCEL)
        dialog.add_button(_("_OK"), Gtk.ResponseType.OK)

        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL,
                       homogeneous=False, spacing=10)
        dialog.get_content_area().add(vbox)
        vbox.show()

        # Create grid to set all the properties inside.
        grid = Gtk.Grid()
        grid.set_column_homogeneous(False)
        grid.set_border_width(10)
        grid.set_column_spacing(10)
        grid.set_row_spacing(10)
        vbox.add(grid)
        grid.show()

        # UI for the file parameter

        def choose_file(widget):
            if file_chooser_dialog.run() == Gtk.ResponseType.OK:
                if file_chooser_dialog.get_file() is not None:
                    config.set_property("file", file_chooser_dialog.get_file())
                    file_entry.set_text(file_chooser_dialog.get_file().get_path())
            file_chooser_dialog.hide()

        file_chooser_button = Gtk.Button.new_with_mnemonic(label=_("_File..."))
        grid.attach(file_chooser_button, 0, 0, 1, 1)
        file_chooser_button.show()
        file_chooser_button.connect("clicked", choose_file)

        file_entry = Gtk.Entry.new()
        grid.attach(file_entry, 1, 0, 1, 1)
        file_entry.set_width_chars(40)
        file_entry.set_placeholder_text(_("Choose export file..."))
        if gio_file is not None:
            file_entry.set_text(gio_file.get_path())
        file_entry.show()

        file_chooser_dialog = Gtk.FileChooserDialog(use_header_bar=use_header_bar,
                                                    title=_("Histogram Export file..."),
                                                    action=Gtk.FileChooserAction.SAVE)
        file_chooser_dialog.add_button(_("_Cancel"), Gtk.ResponseType.CANCEL)
        file_chooser_dialog.add_button(_("_OK"), Gtk.ResponseType.OK)

        # Bucket size parameter
        label = Gtk.Label.new_with_mnemonic(_("_Bucket Size"))
        grid.attach(label, 0, 1, 1, 1)
        label.show()
        spin = GimpUi.prop_spin_button_new(config, "bucket_size", step_increment=0.001, page_increment=0.1, digits=3)
        grid.attach(spin, 1, 1, 1, 1)
        spin.show()

        # Sample average parameter
        spin = GimpUi.prop_check_button_new(config, "sample_average", _("Sample _Average"))
        spin.set_tooltip_text(_("If checked, the histogram is generated from merging all visible layers."
                                " Otherwise, the histogram is only for the current layer."))
        grid.attach(spin, 1, 2, 1, 1)
        spin.show()

        # Output format parameter
        label = Gtk.Label.new_with_mnemonic(_("_Output Format"))
        grid.attach(label, 0, 3, 1, 1)
        label.show()
        combo = GimpUi.prop_string_combo_box_new(config, "output_format", output_format_enum.get_tree_model(), 0, 1)
        grid.attach(combo, 1, 3, 1, 1)
        combo.show()

        progress_bar = Gtk.ProgressBar()
        vbox.add(progress_bar)
        progress_bar.show()

        dialog.show()
        if dialog.run() != Gtk.ResponseType.OK:
            return procedure.new_return_values(Gimp.PDBStatusType.CANCEL,
                                               GLib.Error())

        # Extract values from UI
        gio_file = Gio.file_new_for_path(file_entry.get_text())  # config.get_property("file")
        bucket_size = config.get_property("bucket_size")
        sample_average = config.get_property("sample_average")
        output_format = config.get_property("output_format")

    if gio_file is None:
        error = 'No file given'
        return procedure.new_return_values(Gimp.PDBStatusType.CALLING_ERROR,
                                           GLib.Error(error))

    result = histogram_export(procedure, image, layers, gio_file,
                              bucket_size, sample_average, output_format, progress_bar)

    # If the execution was successful, save parameters so they will be restored next time we show dialog.
    if result.index(0) == Gimp.PDBStatusType.SUCCESS and config is not None:
        config.end_run(Gimp.PDBStatusType.SUCCESS)

    return result


class HistogramExport(Gimp.PlugIn):

    ## Parameters ##
    __gproperties__ = {
        # "filename": (str,
        #              # TODO: I wanted this property to be a path (and not just str) , so I could use
        #              # prop_file_chooser_button_new to open a file dialog. However, it fails without an error message.
        #              # Gimp.ConfigPath,
        #              _("Histogram _File"),
        #              _("Histogram _File"),
        #              "histogram_export.csv",
        #              # Gimp.ConfigPathType.FILE,
        #              GObject.ParamFlags.READWRITE),
        "file": (Gio.File,
                 _("Histogram _File"),
                 "Histogram export file",
                 GObject.ParamFlags.READWRITE),
        "bucket_size":  (float,
                         _("_Bucket Size"),
                         "Bucket Size",
                         0.001, 1.0, 0.01,
                         GObject.ParamFlags.READWRITE),
        "sample_average": (bool,
                           _("Sample _Average"),
                           "Sample Average",
                           False,
                           GObject.ParamFlags.READWRITE),
        "output_format": (str,
                          _("Output format"),
                          "Output format: 'pixel count', 'normalized', 'percent'",
                          "pixel count",
                          GObject.ParamFlags.READWRITE),
    }

    ## GimpPlugIn virtual methods ##
    def do_set_i18n(self, procname):
        return True, 'gimp30-python', None

    def do_query_procedures(self):
        return ['histogram-export']

    def do_create_procedure(self, name):
        procedure = None
        if name == 'histogram-export':
            procedure = Gimp.ImageProcedure.new(self, name,
                                                Gimp.PDBProcType.PLUGIN,
                                                run, None)

            procedure.set_image_types("*")
            procedure.set_documentation (
                N_("Exports the image histogram to a text file (CSV)"),
                globals()["__doc__"],  # This includes the docstring, on the top of the file
                name)
            procedure.set_menu_label(N_("_Export histogram..."))
            procedure.set_attribution("João S. O. Bueno",
                                      "(c) GPL V3.0 or later",
                                      "2014")
            procedure.add_menu_path("<Image>/Colors/Info/")

            procedure.add_argument_from_property(self, "file")
            procedure.add_argument_from_property(self, "bucket_size")
            procedure.add_argument_from_property(self, "sample_average")
            procedure.add_argument_from_property(self, "output_format")

        return procedure


Gimp.main(HistogramExport.__gtype__, sys.argv)
