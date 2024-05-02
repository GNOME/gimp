#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#   GIMP - The GNU Image Manipulation Program
#   Copyright (C) 1995 Spencer Kimball and Peter Mattis
#
#   gimpconfig.py
#   Copyright (C) 2022-2024 Jacob Boerema
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

"""Configuration handling for use with our testing framework."""


import os

class GimpConfig(object):
    DEFAULT_CONFIG        = "config.ini"
    DEFAULT_CONFIG_FOLDER = "/tests/"
    DEFAULT_LOGFILE       = "/tests/gimp-tests.log"
    DEFAULT_DATA_FOLDER   = "/tests/"

    def __init__(self, config_path=None, log_path=None, data_path=None):
        base_path = os.path.dirname(os.path.realpath(__file__))
        if config_path is None:
            config_path = os.getenv("GIMP_TESTS_CONFIG_FILE")
        if log_path is None:
            log_path = os.getenv("GIMP_TESTS_LOG_FILE")
        if data_path is None:
            data_path = os.getenv("GIMP_TESTS_DATA_FOLDER")

        if config_path is None:
            config_path = base_path + self.DEFAULT_CONFIG_FOLDER + self.DEFAULT_CONFIG
            #print(f"Config path for file plug-in tests: {config_path}")

        self.config_folder = os.path.dirname(config_path)
        if os.path.exists(self.config_folder):
            self.config_folder += "/"
        else:
            if self.config_folder is not None:
                print (f"Config folder '{self.config_folder}' does not exist!")
            self.config_folder = self.DEFAULT_CONFIG_FOLDER

        self.config_file = os.path.basename(config_path)
        if self.config_file is None:
            self.config_file = self.DEFAULT_CONFIG

        filepath = self.config_folder + self.config_file
        if not os.path.exists(filepath):
            raise Exception(f"Config file '{filepath}' does not exist!")

        if log_path is None:
            log_path = base_path + self.DEFAULT_LOGFILE

        filepath = os.path.dirname(log_path)
        if not os.path.exists(filepath):
            if filepath is not None:
                print (f"Log file path '{filepath}' does not exist!")
            self.log_file = self.DEFAULT_LOGFILE
        else:
            self.log_file = log_path

        # xml Junit-like results use name and path of log file, but with
        # an xml extension
        self.xml_results_file = os.path.splitext(self.log_file)[0] + '.xml'

        if data_path is None:
            data_path = base_path + self.DEFAULT_DATA_FOLDER

        self.data_folder = data_path
        if not os.path.exists(self.data_folder):
            if self.data_folder is not None:
                print (f"Data path for test images '{self.data_folder}' does not exist!")
            self.data_folder = self.DEFAULT_DATA_FOLDER
