#!/usr/bin/env python3

"""
performance-log-resolve.py -- Resolve GIMP performance log backtrace symbols
Copyright (C) 2018  Ell

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


Usage: performance-log-resolve.py < infile > outfile
"""

from xml.etree import ElementTree
import sys

empty_element = ElementTree.Element ("")

# Read performance log from STDIN
log = ElementTree.fromstring (sys.stdin.buffer.read ())

address_map = log.find ("address-map")

if address_map:
    # Create address dictionary
    addresses = {}

    for address in address_map.iterfind ("address"):
        addresses[address.get ("value")] = list (address)

    # Resolve addresses in backtraces
    for sample in (log.find ("samples") or empty_element).iterfind ("sample"):
        for thread in sample.find ("backtrace") or ():
            for frame in thread:
                address = addresses.get (frame.get ("address"))

                if address:
                    frame.text = "\n"
                    frame.extend (address)

    # Remove address map
    log.remove (address_map)

# Write performance log to STDOUT
sys.stdout.buffer.write (ElementTree.tostring (log))
