#!/usr/bin/env python3

"""
performance-log-expand.py -- Delta-decodes GIMP performance logs
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


Usage: performance-log-expand.py < infile > outfile
"""

from xml.etree import ElementTree
import sys

empty_element = ElementTree.Element ("")

# Read performance log from STDIN
log = ElementTree.fromstring (sys.stdin.buffer.read ())

try:
    has_backtrace = bool (int (log.find ("params").find ("backtrace").text))
except:
    has_backtrace = False

def expand_simple (element, last_values):
    last_values.update ({value.tag: value.text for value in element})

    for value in list (element):
        element.remove (value)

    for tag, text in last_values.items ():
        value = ElementTree.SubElement (element, tag)
        value.text = text
        value.tail = "\n"

# Expand samples
last_vars      = {}
last_backtrace = {}

for sample in (log.find ("samples") if log.find ("samples") is not None else empty_element).iterfind ("sample"):
    # Expand variables
    vars = sample.find ("vars")
    if vars is None:
        vars = ElementTree.SubElement (sample, "vars")

    expand_simple (vars, last_vars)

    # Expand backtrace
    if has_backtrace:
        backtrace = sample.find ("backtrace")
        if backtrace is None:
            backtrace = ElementTree.SubElement (sample, "backtrace")

        for thread in backtrace:
            id     = thread.get ("id")
            head   = thread.get ("head")
            tail   = thread.get ("tail")
            attrib = dict (thread.attrib)
            frames = list (thread)

            last_thread = last_backtrace.setdefault (id, [{}, []])
            last_frames = last_thread[1]

            if head:
                frames = last_frames[:int (head)] + frames

                del attrib["head"]

            if tail:
                frames = frames + last_frames[-int (tail):]

                del attrib["tail"]

            last_thread[0] = attrib
            last_thread[1] = frames

            if not frames and thread.text is None:
                del last_backtrace[id]

        for thread in list (backtrace):
            backtrace.remove (thread)

        for id, (attrib, frames) in last_backtrace.items ():
            thread = ElementTree.SubElement (backtrace, "thread", attrib)
            thread.text = "\n"
            thread.tail = "\n"

            thread.extend (frames)

# Expand address map
last_address = {}

for address in (log.find ("address-map") if log.find ("address-map") is not None else empty_element).iterfind ("address"):
    expand_simple (address, last_address)

# Write performance log to STDOUT
sys.stdout.buffer.write (ElementTree.tostring (log))
