#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#   GIMP - The GNU Image Manipulation Program
#   Copyright (C) 1995 Spencer Kimball and Peter Mattis
#
#   batch-import-tests.py
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

"""Batch non-interactive GIMP file import plug-ins testing."""

from gimpconfig import GimpConfig
from gimplogger import GimpLogger
from gimptestframework import GimpTestRunner


#DEBUGGING=True
DEBUGGING=False

#PRINT_VERBOSE = True
PRINT_VERBOSE = False

LOG_APPEND = False

test_cfg = GimpConfig()

log = GimpLogger(False, test_cfg.log_file, LOG_APPEND, PRINT_VERBOSE, DEBUGGING)

runner = GimpTestRunner(log, "import", test_cfg)
runner.run_tests()
