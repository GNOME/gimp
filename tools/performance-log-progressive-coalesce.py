#!/usr/bin/env python3

"""
performance-log-progressive-coalesce.py -- Coalesce address-maps in progressive
GIMP performance logs
Copyright (C) 2020  Ell

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.


Usage: performance-log-progressive-coalesce.py < infile > outfile
"""

from xml.etree import ElementTree
import sys

empty_element = ElementTree.Element ("")

# Read performance log from STDIN
log = ElementTree.fromstring (sys.stdin.buffer.read ())

samples = log.find ("samples")
if samples is None:
  samples = empty_element

address_map = log.find ("address-map")

if address_map is None:
  address_map = ElementTree.Element ("address-map")

# Coalesce partial address maps
for partial_address_map in samples.iterfind ("address-map"):
  for element in partial_address_map:
    address_map.append (element)

# Remove partial address maps
for partial_address_map in samples.iterfind ("address-map"):
  samples.remove (partial_address_map)

# Add global address map
if log.find ("address-map") is None and len (address_map) > 0:
  log.append (address_map)

# Write performance log to STDOUT
sys.stdout.buffer.write (ElementTree.tostring (log))
