#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#   GIMP - The GNU Image Manipulation Program
#   Copyright (C) 1995 Spencer Kimball and Peter Mattis
#
#   gimpexporttests.py
#   Copyright (C) 2021-2024 Jacob Boerema
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

"""Module where we add export tests for GIMP's file export plug-ins."""

import sys

import gi
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp
from gi.repository import Gio

from gimptestframework import GimpExportTest


class BmpExportTests(GimpExportTest):
    def __init__(self, group_name, log):
        super().__init__(group_name, log)

    def load_bmp(self, bmp_name):
        pdb_proc   = Gimp.get_pdb().lookup_procedure(self.file_import)
        pdb_config = pdb_proc.create_config()
        pdb_config.set_property('run-mode', Gimp.RunMode.NONINTERACTIVE)
        pdb_config.set_property('file', Gio.File.new_for_path(bmp_name))
        result = pdb_proc.run(pdb_config)
        status = result.index(0)
        img = result.index(1)

        if (status == Gimp.PDBStatusType.SUCCESS and img is not None and img.is_valid()):
            return img
        else:
            self.log.error(f"Loading image {bmp_name} failed!")
            return None

    def export_bmp(self, img, bmp_exp_name, rle, space, rgb):
        drw = img.get_layers()
        export_file_path = self.output_path + bmp_exp_name

        pdb_proc   = Gimp.get_pdb().lookup_procedure(self.file_export)
        pdb_config = pdb_proc.create_config()
        pdb_config.set_property('run-mode', Gimp.RunMode.NONINTERACTIVE)
        pdb_config.set_property('image', img)
        pdb_config.set_property('num-drawables', 1)
        pdb_config.set_property('drawables', Gimp.ObjectArray.new(Gimp.Drawable, drw, False))
        pdb_config.set_property('file', Gio.File.new_for_path(export_file_path))
        pdb_config.set_property('use-rle', rle)
        pdb_config.set_property('write-color-space', space)
        pdb_config.set_property('rgb-format', rgb)

        result = pdb_proc.run(pdb_config)
        status = result.index(0)
        if status == Gimp.PDBStatusType.SUCCESS:
            self.log.info(f"Exporting image {export_file_path} succeeded.")
        else:
            self.log.error(f"Exporting image {export_file_path} failed!")

    def bmp_setup(self, image_name, test_description):
        self.log.consoleinfo(test_description)
        tmp = self.input_path + image_name
        img = self.load_bmp(tmp)
        return img

    def bmp_teardown(self, image):
        if image is not None:
            image.delete()

    def bmp_1bpp_indexed(self, image_name, test_description):
        img = self.bmp_setup(image_name, test_description)
        if img is not None:
            self.export_bmp(img, "1bpp-norle-nospace.bmp", 0, 0, 0)
            self.export_bmp(img, "1bpp-norle-withspace.bmp", 0, 1, 0)
            # rle with 1-bit is not valid, test to see if it
            # is correctly ignored...
            self.export_bmp(img, "1bpp-rle-nospace.bmp", 1, 0, 0)
            self.export_bmp(img, "1bpp-rle-withspace.bmp", 1, 1, 0)
        self.bmp_teardown(img)

    def bmp_4bpp_indexed(self, image_name, test_description):
        img = self.bmp_setup(image_name, test_description)
        if img is not None:
            self.export_bmp(img, "4bpp-norle-nospace.bmp", 0, 0, 0)
            self.export_bmp(img, "4bpp-norle-withspace.bmp", 0, 1, 0)
            self.export_bmp(img, "4bpp-rle-nospace.bmp", 1, 0, 0)
            self.export_bmp(img, "4bpp-rle-withspace.bmp", 1, 1, 0)
        self.bmp_teardown(img)

    def bmp_8bpp_indexed(self, image_name, test_description):
        img = self.bmp_setup(image_name, test_description)
        if img is not None:
            self.export_bmp(img, "8bpp-norle-nospace.bmp", 0, 0, 0)
            self.export_bmp(img, "8bpp-norle-withspace.bmp", 0, 1, 0)
            self.export_bmp(img, "8bpp-rle-nospace.bmp", 1, 0, 0)
            self.export_bmp(img, "8bpp-rle-withspace.bmp", 1, 1, 0)
        self.bmp_teardown(img)

    def bmp_rgba(self, image_name, test_description):
        img = self.bmp_setup(image_name, test_description)
        if img is not None:
            # first test correctly ignoring rle
            self.export_bmp(img, "rgb-with-rle-nospace.bmp", 1, 0, 3)
            # All rgb modes without color space
            self.export_bmp(img, "rgb-nrle-nospace-type0.bmp", 0, 0, 0)
            self.export_bmp(img, "rgb-nrle-nospace-type1.bmp", 0, 0, 1)
            self.export_bmp(img, "rgb-nrle-nospace-type2.bmp", 0, 0, 2)
            self.export_bmp(img, "rgb-nrle-nospace-type3.bmp", 0, 0, 3)
            self.export_bmp(img, "rgb-nrle-nospace-type4.bmp", 0, 0, 4)
            self.export_bmp(img, "rgb-nrle-nospace-type5.bmp", 0, 0, 5)
            # All rgb modes with color space
            self.export_bmp(img, "rgb-nrle-withspace-type0.bmp", 0, 1, 0)
            self.export_bmp(img, "rgb-nrle-withspace-type1.bmp", 0, 1, 1)
            self.export_bmp(img, "rgb-nrle-withspace-type2.bmp", 0, 1, 2)
            self.export_bmp(img, "rgb-nrle-withspace-type3.bmp", 0, 1, 3)
            self.export_bmp(img, "rgb-nrle-withspace-type4.bmp", 0, 1, 4)
            self.export_bmp(img, "rgb-nrle-withspace-type5.bmp", 0, 1, 5)
        self.bmp_teardown(img)

    def run_test(self, group_config):
        self.bmp_1bpp_indexed('test1-exp1.bmp',   "    1. Using 1bpp indexed BMP...")
        self.bmp_4bpp_indexed('pal4rle-exp1.bmp', "    2. Using 4bpp indexed BMP...")
        self.bmp_8bpp_indexed('pal8v5-exp1.bmp',  "    3. Using 8bpp indexed BMP...")
        self.bmp_rgba('rgb32-with-alpha-1.bmp',   "    4. Using rgb(a) BMP...")
