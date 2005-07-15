#!/usr/bin/env python

#   Foreground Extraction Benchmark
#   Copyright 2005  Sven Neumann <sven@gimp.org>
#
#   This is a from-scratch implementation of the benchmark proposed in
#   "GrabCut": interactive foreground extraction using iterated graph
#   cuts published in the Proceedings of the 2004 SIGGRAPH Conference.
#
#   No guarantee is made that this benchmark produces the same results
#   as the cited benchmark but the goal is that it does. So if you find
#   any bugs or inaccuracies in this code, please let us know.
#
#   The benchmark has been adapted work with the SIOX algorithm
#   (http://www.siox.org). which is (currently) the only
#   implementation of gimp_drawable_foreground_extract(). If other
#   implementations are being added, this benchmark should be changed
#   accordingly.
#
#   You will need a set of test images to run this benchmark, preferably
#   the original set of 50 images. Some of these images are from the
#   Berkeley Segmentation Dataset
#   (http://www.cs.berkeley.edu/projects/vision/grouping/segbench/).
#   See also http://www.siox.org/details.html for trimaps.
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


import os, re, struct, sys, time

from gimpfu import *


def benchmark (folder):
    folder = os.path.abspath (folder)
    if not os.path.exists (folder):
        gimp.message("Folder '" + folder + "' doesn't exist.\n")
        return;
    
    images = os.path.join (folder, "images")
    masks = os.path.join (folder, "cm_bmp")
    truths = os.path.join (folder, "truth")

    total_unclassified = 0
    total_misclassified = 0

    for name in os.listdir (images):

        try:
            gimp.delete (image_display)
            gimp.delete (mask_display)
        except UnboundLocalError:
            pass

        image_name = os.path.join (images, name)

        # FIXME: improve this!
        name = re.sub (r'\.jpg$', '', name)
        name = re.sub (r'\.JPG$', '', name)
        name = re.sub (r'\.bmp$', '', name)

        mask_name = os.path.join (masks, name + '.png')
        truth_name = os.path.join (truths, name + '.bmp')

        image = pdb.gimp_file_load (image_name, image_name)
        image_layer = image.active_layer;

        mask = pdb.gimp_file_load (mask_name, mask_name)
        pdb.gimp_image_convert_grayscale (mask)
        mask_layer = mask.active_layer;

        truth = pdb.gimp_file_load (truth_name, truth_name)

	gimp.tile_cache_ntiles (2 * mask_layer.width / gimp.tile_width () + 1)

        unclassified = ((mask_layer.width * mask_layer.height)
			- classified_pixels (mask_layer))

        sys.stderr.write (os.path.basename (image_name))

        pdb.gimp_drawable_foreground_extract (image_layer, mask_layer)

        sys.stderr.write (" ")

	mask_layer.flush ()

        # Ignore errors when creating image displays;
        # allows us to be used without a display.
        try:
            image_display = pdb.gimp_display_new (image)
            mask_display = pdb.gimp_display_new (mask)

            gimp.displays_flush ()
            time.sleep (1.0)
        except:
            pass

        gimp.delete (image)

        misclassified = different_pixels (mask_layer, truth.active_layer)

        sys.stderr.write ("%d %d %.2f%%\n" %
			  (unclassified, misclassified,
			   (misclassified * 100.0 / unclassified)))

	total_unclassified += unclassified
	total_misclassified += misclassified

        gimp.delete (mask)
        gimp.delete (truth)

    # done

    try:
	gimp.delete (image_display)
	gimp.delete (mask_display)
    except UnboundLocalError:
	pass

    sys.stderr.write ("Total: %d %d %.2f%%\n" %
		      (total_unclassified, total_misclassified,
		       (total_misclassified * 100.0 / total_unclassified)))


def classified_pixels (mask):
    (mean, std_dev, median, pixels, count, percentile) = pdb.gimp_histogram (mask, HISTOGRAM_VALUE, 224, 255)

    return count


def different_pixels (mask, truth):
    image = truth.image

    copy = pdb.gimp_layer_new_from_drawable (mask, image)
    copy.name = "Difference"
    copy.mode = DIFFERENCE_MODE

    image.add_layer (copy, -1)

    (mean, std_dev, median,
     pixels, count, percentile) = pdb.gimp_histogram (image.flatten (),
						      HISTOGRAM_VALUE, 32, 255)

    return count


register (
    "foreground_extraction_benchmark",
    "Foreground Extraction Benchmark",
    "Foreground Extraction Benchmark",
    "Sven Neumann",
    "Sven Neumann",
    "2005",
    "<Toolbox>/Xtns/Benchmark/Foreground Extraction",
    "",
    [ (PF_FILE, "image_folder", "Image folder",
                "~/segmentation/msbench/imagedata") ],
    [],
    benchmark)

main ()
