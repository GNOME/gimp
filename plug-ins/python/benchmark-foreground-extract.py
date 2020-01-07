#!/usr/bin/env python3

#   Foreground Extraction Benchmark
#   Copyright 2005  Sven Neumann <sven@gimp.org>
#
"""
  This is a from-scratch implementation of the benchmark proposed in
  "GrabCut": interactive foreground extraction using iterated graph
  cuts published in the Proceedings of the 2004 SIGGRAPH Conference.

  No guarantee is made that this benchmark produces the same results
  as the cited benchmark but the goal is that it does. So if you find
  any bugs or inaccuracies in this code, please let us know.

  The benchmark has been adapted work with the MATTING algorithm,
  which is (currently) the only
  implementation of gimp_drawable_foreground_extract(). If other
  implementations are being added, this benchmark should be changed
  accordingly.

  You will need a set of test images to run this benchmark, preferably
  the original set of 50 images. Some of these images are from the
  Berkeley Segmentation Dataset
  http://www.cs.berkeley.edu/projects/vision/grouping/segbench/ .
  See http://www.siox.org/details.html to download trimaps.
  See https://web.archive.org/web/20050209123253/http://research.microsoft.com/vision/cambridge/segmentation/
  and download the "Labelling - Lasso" file.
"""
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

import os, re, struct, sys, time

import gi
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio


def benchmark (procedure, args, data):
    if args.length() != 3:
        error = 'Wrong parameters given'
        return procedure.new_return_values(Gimp.PDBStatusType.CALLING_ERROR,
                                           GLib.Error(error))
    run_mode = args.index(0)
    folder = args.index(1)
    save_output = args.index(2)

    folder = os.path.abspath(os.path.expanduser(folder))
    if not os.path.exists(folder):
        error = "Folder '" + folder + "' doesn't exist.\n"
        return procedure.new_return_values(Gimp.PDBStatusType.CALLING_ERROR,
                                           GLib.Error(error))

    total_unclassified = 0
    total_misclassified = 0
    total_time = 0.0

    images = os.path.join(folder, "images")
    for name in os.listdir(images):

        try:
            image_display.delete()
            mask_display.delete()
        except NameError:
            pass

        image_name = os.path.join (images, name)

        # Remove suffix, assuming it has three characters
        name = re.sub(r'\....$', '', name)

        mask_name = os.path.join(folder, "cm_bmp", name + '.png')
        truth_name = os.path.join(folder, "truth", name + '.bmp')

        image = Gimp.file_load(run_mode, Gio.file_new_for_path(image_name))
        image_layer = image.get_active_layer()

        mask = Gimp.file_load(run_mode, Gio.file_new_for_path(mask_name))
        convert_grayscale(mask)
        mask_layer = mask.get_active_layer()

        truth = Gimp.file_load(run_mode, Gio.file_new_for_path(truth_name))
        convert_grayscale(truth)
        truth_layer = truth.get_active_layer()

        unclassified = unclassified_pixels(mask_layer, truth_layer)

        sys.stderr.write(os.path.basename (image_name))

        start = time.time()
        image_layer.foreground_extract(Gimp.ForegroundExtractMode.MATTING, mask_layer)
        end = time.time()

        sys.stderr.write(" ")

        # This line was in the gimp 2 implementation, and probably isn't needed anymore.
        #  mask_layer.flush ()

        # Ignore errors when creating image displays;
        # allows us to be used without a display.
        try:
            image_display = Gimp.Display.new(image)
            mask_display = Gimp.Display.new(mask)

            Gimp.displays_flush()
            time.sleep(1.0)
        except:
            pass

        image.delete()

        misclassified = misclassified_pixels (mask_layer, truth_layer)

        sys.stderr.write("%d %d %.2f%% %.3fs\n" %
                         (unclassified, misclassified,
                          (misclassified * 100.0 / unclassified),
                          end - start))

        total_unclassified += unclassified
        total_misclassified += misclassified
        total_time += end - start

        truth.delete()

        if save_output:
            filename = os.path.join(folder, "output", name + '.png')
            Gimp.file_save(Gimp.RunMode.NONINTERACTIVE, mask, mask_layer, Gio.file_new_for_path(filename))

        mask.delete()

    # for loop ends

    try:
        image_display.delete()
        mask_display.delete()
    except NameError:
        pass

    sys.stderr.write("Total: %d %d %.2f%% %.3fs\n" %
                     (total_unclassified, total_misclassified,
                       (total_misclassified * 100.0 / total_unclassified),
                       total_time))

    return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())


def convert_grayscale(image):
    if not image.get_effective_color_profile().is_gray():
        image.convert_grayscale()


def unclassified_pixels(mask, truth):
    (result, mean, std_dev, median, pixels,
     count, percentile) = mask.histogram(Gimp.HistogramChannel.VALUE, 2/256.0, 254/256.0)

    return count


def misclassified_pixels(mask, truth):
    image = truth.get_image()

    copy = Gimp.Layer.new_from_drawable(mask, image)
    copy.set_name("Difference")
    copy.set_mode(Gimp.LayerMode.DIFFERENCE_LEGACY)

    image.insert_layer(copy, None, -1)

    # The assumption made here is that the output of
    # foreground_extract is a strict black and white mask. The truth
    # however may contain unclassified pixels. These are considered
    # unknown, a strict segmentation isn't possible here.
    #
    # The result of using the Difference mode as done here is that
    # pure black pixels in the result can be considered correct.
    # White pixels are wrong. Gray values were unknown in the truth
    # and thus are not counted as wrong.

    flat_image = image.flatten()
    (result, mean, std_dev, median, pixels,
     count, percentile) = flat_image.histogram(Gimp.HistogramChannel.VALUE, 254/256.0, 1.0)

    return count


PROCNAME = "python-fu-benchmark-foreground-extract"

class BenchmarkForegroundExtract(Gimp.PlugIn):

    ## Parameters ##
    __gproperties__ = {
        "run-mode": (Gimp.RunMode,
                     "Run mode",
                     "The run mode",
                     Gimp.RunMode.NONINTERACTIVE,
                     GObject.ParamFlags.READWRITE),
        "image_folder": (str,
                        "Image Folder",
                        "Image Folder",
                        "~/segmentation/msbench/imagedata",
                        GObject.ParamFlags.READWRITE),
        "save_output": (bool,
                        "Save output images",
                        "Save output images",
                        False,
                        GObject.ParamFlags.READWRITE)
    }

    ## GimpPlugIn virtual methods ##
    def do_query_procedures(self):
        self.set_translation_domain("gimp30-python",
                                    Gio.file_new_for_path(Gimp.locale_directory()))
        return [PROCNAME]

    def do_create_procedure(self, name):
        procedure = None
        if name == PROCNAME:
            procedure = Gimp.Procedure.new(self, name,
                                           Gimp.PDBProcType.PLUGIN,
                                           benchmark, None)
            procedure.set_documentation(
                "Benchmark and regression test for Foreground Extraction",
                globals()["__doc__"],  # This includes the docstring, on the top of the file
                name)
            procedure.set_menu_label("Foreground Extraction")
            procedure.set_attribution("Sven Neumann",
                                      "Sven Neumann",
                                      "2005")
            procedure.add_menu_path("<Image>/Filters/Extensions/Benchmark")

            procedure.add_argument_from_property(self, "run-mode")
            procedure.add_argument_from_property(self, "image_folder")
            procedure.add_argument_from_property(self, "save_output")

        return procedure


Gimp.main(BenchmarkForegroundExtract.__gtype__, sys.argv)
