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

def histogram_export(procedure, img, layers, gio_file,
                     bucket_size, sample_average, output_format):
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
        with open(gio_file.get_path(), "wt", newline="") as hfile:
            writer = csv.writer(hfile)
            histo_proc = Gimp.get_pdb().lookup_procedure('gimp-drawable-histogram')
            histo_config = histo_proc.create_config()
            histo_config.set_property('drawable', layer)

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

                histo_config.set_property('start-range', float(start_range))
                histo_config.set_property('end-range', float(min(start_range + bucket_size, 1.0)))
                row = [start_range]
                for channel in channels_gimp:
                    histo_config.set_property('channel', channel)
                    result = histo_proc.run(histo_config)

                    if output_format == "pixel-count":
                        count = int(result.index(5))
                    else:
                        pixels = result.index(4)
                        count = (result.index(5) / pixels) if pixels else 0
                        if output_format == "percent":
                            count = "%.2f%%" % (count * 100)
                    row.append(str(count))
                writer.writerow(row)

                # Update progress bar
                fraction = i / max_index
                # Only update the progress bar if it changed at least 1% .
                new_percent = math.floor(fraction * 100)
                if new_percent != progress_bar_int_percent:
                    progress_bar_int_percent = new_percent
                Gimp.progress_update(fraction)
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


def run(procedure, run_mode, image, n_layers, layers, config, data):
    if run_mode == Gimp.RunMode.INTERACTIVE:
        GimpUi.init("histogram-export.py")

        dialog = GimpUi.ProcedureDialog.new(procedure, config, _("Histogram Export..."))

        radio_frame = dialog.get_widget("output-format", GimpUi.IntRadioFrame)

        dialog.fill(None)

        if not dialog.run():
            return procedure.new_return_values(Gimp.PDBStatusType.CANCEL,
                                               GLib.Error())

    gio_file       = config.get_property('file')
    bucket_size    = config.get_property("bucket-size")
    sample_average = config.get_property("sample-average")
    output_format  = config.get_property("output-format")

    if gio_file is None:
        error = 'No file given'
        return procedure.new_return_values(Gimp.PDBStatusType.CALLING_ERROR,
                                           GLib.Error(error))

    result = histogram_export(procedure, image, layers, gio_file,
                              bucket_size, sample_average, output_format)

    return result


class HistogramExport(Gimp.PlugIn):
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
                _("Exports the image histogram to a text file (CSV)"),
                globals()["__doc__"],  # This includes the docstring, on the top of the file
                name)
            procedure.set_menu_label(_("_Export histogram..."))
            procedure.set_attribution("João S. O. Bueno",
                                      "(c) GPL V3.0 or later",
                                      "2014")
            procedure.add_menu_path("<Image>/Colors/Info/")

            # TODO: GFile props still don't have labels + only load existing files
            # (here we likely want to create a new file).
            procedure.add_file_argument ("file", _("Histogram File"),
                                         _("Histogram export file"), GObject.ParamFlags.READWRITE)
            procedure.add_double_argument ("bucket-size", _("_Bucket Size"), _("Bucket Size"),
                                           0.001, 1.0, 0.01, GObject.ParamFlags.READWRITE)
            procedure.add_boolean_argument ("sample-average", _("Sample _Average"), _("Sample Average"),
                                            False, GObject.ParamFlags.READWRITE)
            choice = Gimp.Choice.new()
            choice.add("pixel-count", 0, _("Pixel Count"), "")
            choice.add("normalized", 1, _("Normalized"), "")
            choice.add("percent", 2, _("Percent"), "")
            procedure.add_choice_argument ("output-format", _("Output _format"), _("Output format"),
                                           choice, "percent", GObject.ParamFlags.READWRITE)

        return procedure


Gimp.main(HistogramExport.__gtype__, sys.argv)
