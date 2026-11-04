#!/usr/bin/env python3

# get-merge-request-id.py - utility for GIMP CI.
# Copyright (C) 2026 Jehan
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import os
import sys

# In merge requests, this environment variable will always be set. As a
# special fallback, we also look at a file in .gitlab/, for Snap and
# Flatpak which are compiled in a sandbox without access to the host
# environment.
mr = os.getenv("CI_MERGE_REQUEST_IID")
if mr is None:
  root = os.getenv('MESON_SOURCE_ROOT')
  try:
    with open(os.path.join(root, '.gitlab/CI_MERGE_REQUEST_IID'), mode='r') as f:
      mr = f.read()
  except FileNotFoundError:
    pass

print(int(mr) if mr is not None else 0, end="")
