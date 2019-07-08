#!/usr/bin/env python2
#coding: utf-8

#
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

The first two collums are the bucket boundaries,
followed by the selected columns. The histogram
refers to the selected image area, and
can use either Sample Average data or data
from the current drawable only.;

The output is in "weighted pixels" - meaning
all fully transparent pixels are not counted.

Check the gimp-histogram call
"""


from gimpfu import *
import csv
import gettext


gettext.install("gimp20-python", gimp.locale_directory, unicode=True)

def histogram_export(img, drw, filename,
                     bucket_size, sample_average, output_format):
    if sample_average:
        new_img = pdb.gimp_image_duplicate(img)
        drw = pdb.gimp_image_merge_visible_layers(new_img, CLIP_TO_IMAGE)
    # TODO: grey images, alpha and non alpha images.
    channels_txt = ["Value"] 
    channels_gimp = [HISTOGRAM_VALUE]
    if drw.is_rgb:
        channels_txt += ["Red", "Green", "Blue"]
        channels_gimp += [HISTOGRAM_RED, HISTOGRAM_GREEN, HISTOGRAM_BLUE]
    if drw.has_alpha:
        channels_txt += ["Alpha"]
        channels_gimp += [HISTOGRAM_ALPHA]
    with open(filename, "wt") as hfile:
        writer = csv.writer(hfile)
        #headers:
        writer.writerow(["Range start"] + channels_txt)

        # FIXME: Will need a specialized 'range' for FP color numbers
        for start_range in range(0, 256, int(bucket_size)):
            row = [start_range]
            for channel in channels_gimp:
                result = pdb.gimp_histogram(
                             drw, channel,
                             start_range,
                             min(start_range + bucket_size, 255)
                            )
                if output_format == "pixel count":
                    count = result[4]
                else:
                    count = (result[4] / result[3]) if result[3] else 0
                    if output_format == "percent":
                        count = "%.2f%%" % (count * 100)
                row.append(str(count))
            writer.writerow(row)
    if sample_average:
        pdb.gimp_image_delete(new_img)

register(
    "histogram-export",
    N_("Exports the image histogram to a text file (CSV)"),
    globals()["__doc__"], # This includes the docstring, on the top of the file
    "João S. O. Bueno",
    "João S. O. Bueno, 2014",
    "2014",
    N_("_Export histogram..."),
    "*",
    [(PF_IMAGE,  "img", _("_Image"), None),
     (PF_DRAWABLE, "drw", _("_Drawable"), None),
     (PF_FILENAME, "filename", _("Histogram _File"), ""),
     (PF_FLOAT, "bucket_size", _("_Bucket Size"), 1.0),
     (PF_BOOL, "sample_average", _("Sample _Average"), False),
     (PF_RADIO, "output_format", _("Output format"), "pixel count",
            ((_("Pixel count"), "pixel count"),
             (_("Normalized"), "normalized"),
             (_("Percent"), "percent"),
            )
     )
    ],
    [],
    histogram_export,
    menu="<Image>/Colors/Info",
    domain=("gimp20-python", gimp.locale_directory)
    )

main()
