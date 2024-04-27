#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#   GIMP - The GNU Image Manipulation Program
#   Copyright (C) 1995 Spencer Kimball and Peter Mattis
#
#   gimplogger.py
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

"""Logging framework for use with GIMP Python plug-ins."""

import os
import logging

import gi
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp


class GimpLogger (object):
    def __init__(self, interactive, logfile, append=False, verbose=False, debugging=False):
        self.interactive = interactive

        self.verbose = verbose
        self.debugging = debugging
        self.enabled = True

        if debugging:
            log_level = logging.DEBUG
        else:
            log_level = logging.INFO
        if append:
            log_filemode = 'a'
        else:
            log_filemode = 'w'

        try:
            logging.basicConfig(
                filename=logfile,
                filemode=log_filemode,
                encoding='utf-8',
                level=log_level,
                format='%(asctime)s | %(name)s | %(levelname)s | %(message)s')

            logging.debug("Starting logger...")
            logging.debug("Log file: %s", os.path.abspath(logfile))
        except PermissionError:
            self.enabled = False
            msg = ("We do not have permission to create a log file at " +
                   os.path.abspath(logfile) + ". " +
                   "Please use env var GIMP_TESTS_LOG_FILE to set up a location.")
            if interactive:
                Gimp.message(msg)
            else:
                print(msg)

    def set_interactive(self, interactive):
        if self.interactive != interactive:
            self.interactive = interactive
            logging.debug("Interactive set to %s", self.interactive)

    def message(self, msg):
        logging.info(msg)
        print(msg)
        if self.interactive:
            Gimp.message(msg)

    def gimp_verbose_message(self, msg):
        if self.verbose and self.interactive:
            Gimp.message(msg)

    def info(self, msg):
        if self.verbose:
            logging.info(msg)
            print(msg)

    def consoleinfo(self, msg):
        logging.info(msg)
        print(msg)

    def warning(self, msg):
        warn_msg = 'WARNING: ' + msg
        logging.warning(msg)
        print(warn_msg)
        if self.interactive:
            Gimp.message(warn_msg)

    def error(self, msg):
        err_msg = 'ERROR: ' + msg
        logging.error(msg)
        print(err_msg)
        if self.interactive:
            Gimp.message(err_msg)

    def debug(self, msg):
        logging.debug(msg)
        if self.debugging:
            print(msg)
