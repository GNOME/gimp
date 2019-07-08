#!/usr/bin/env python3

"""
performance-log-coalesce.py -- Coalesce GIMP performance log symbols, by
                               filling-in missing base addresses
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


Usage: performance-log-coalesce.py < infile > outfile
"""

from xml.etree import ElementTree
import sys

empty_element = ElementTree.Element ("")

# Read performance log from STDIN
log = ElementTree.fromstring (sys.stdin.buffer.read ())

address_map = log.find ("address-map")

if address_map:
    addresses = []

    # Create base addresses dictionary
    base_addresses = {}

    for address in address_map.iterfind ("address"):
        symbol = address.find ("symbol").text
        source = address.find ("source").text
        base   = address.find ("base").text

        if symbol and source and not base:
            key   = (symbol, source)
            value = int (address.get ("value"), 0)

            base_addresses[key] = min (value, base_addresses.get (key, value))

            addresses.append (address)

    # Fill-in missing base addresses
    for address in addresses:
        symbol = address.find ("symbol").text
        source = address.find ("source").text

        address.find ("base").text = hex (base_addresses[(symbol, source)])

# Write performance log to STDOUT
sys.stdout.buffer.write (ElementTree.tostring (log))
