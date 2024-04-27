#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#   GIMP - The GNU Image Manipulation Program
#   Copyright (C) 1995 Spencer Kimball and Peter Mattis
#
#   test-file-plug-ins.py
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

"""Plug-in to test GIMP file import plug-ins from within GIMP."""

import sys

import gi
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp

from gi.repository import GLib

from gimpconfig import GimpConfig
from gimplogger import GimpLogger
from gimptestframework import GimpTestRunner, GimpExportTestRunner, AUTHORS, YEARS, VERSION
from gimpexporttests import BmpExportTests


#DEBUGGING=True
DEBUGGING=False

#PRINT_VERBOSE = True
PRINT_VERBOSE = False

LOG_APPEND = False


class PythonTest (Gimp.PlugIn):

    def __init__(self):
        Gimp.PlugIn.__init__(self)
        self.test_cfg = None
        self.log = None

    ## GimpPlugIn virtual methods ##
    def do_set_i18n(self, _name):
        # We don't support internationalization here...
        return False

    def do_query_procedures(self):
        return [ 'test-import-plug-ins',
                 'test-export-plug-ins' ]

    def do_create_procedure(self, name):

        if name == 'test-import-plug-ins':
            procedure = Gimp.ImageProcedure.new(self, name,
                                                Gimp.PDBProcType.PLUGIN,
                                                self.run_import_tests, None)
            procedure.set_image_types("*")
            procedure.set_sensitivity_mask(Gimp.ProcedureSensitivityMask.ALWAYS)
            procedure.set_menu_label('Test file _import plug-ins')
            procedure.add_menu_path('<Image>/Filters/Development/Python-Fu/')
            procedure.set_documentation('Run file import plug-in tests',
                                        'Run file import plug-in tests',
                                        name)
        elif name == 'test-export-plug-ins':
            procedure = Gimp.ImageProcedure.new(self, name,
                                                Gimp.PDBProcType.PLUGIN,
                                                self.run_export_tests, None)
            procedure.set_image_types("*")
            procedure.set_sensitivity_mask(Gimp.ProcedureSensitivityMask.ALWAYS)
            procedure.set_menu_label('Test file _export plug-ins')
            procedure.add_menu_path('<Image>/Filters/Development/Python-Fu/')
            procedure.set_documentation('Run file export plug-in tests',
                                        'Run file export plug-in tests',
                                        name)
        else:
            return None

        procedure.set_attribution(AUTHORS, #author
                                  AUTHORS, #copyright
                                  YEARS)   #year
        return procedure

    def init_logging(self):
        self.test_cfg = GimpConfig()

        self.log = GimpLogger(True, self.test_cfg.log_file, LOG_APPEND, PRINT_VERBOSE, DEBUGGING)

    def run_import_tests(self, procedure, _run_mode, _image,
                         _n_drawables, _drawable, _config, _data):
        self.init_logging()

        if self.log.enabled:
            runner = GimpTestRunner(self.log, "import", self.test_cfg)
            runner.run_tests()

        return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())

    def run_export_tests(self, procedure, _run_mode, _image,
                         _n_drawables, _drawable, _config, _data):
        self.init_logging()

        if self.log.enabled:
            runner = GimpExportTestRunner(self.log, "export", self.test_cfg)
            if not runner:
                self.log.error("Failed to create export test runner!")
            else:
                runner.load_test_configs()

                bmp_tests = BmpExportTests("bmp", self.log)
                runner.add_test(bmp_tests)

                # Add additional tests here

                runner.run_tests()

        return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())

Gimp.main(PythonTest.__gtype__, sys.argv)
