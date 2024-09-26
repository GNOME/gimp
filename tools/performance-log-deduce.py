#!/usr/bin/env python3

"""
performance-log-deduce.py -- Deduce GIMP performance log thread state
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


Usage: performance-log-deduce.py < infile > outfile
"""

DEDUCE_MIN_N_OCCURRENCES = 10
DEDUCE_MIN_PERCENTAGE    = 0.75

from xml.etree import ElementTree
import sys

empty_element = ElementTree.Element ("")

# Read performance log from STDIN
log = ElementTree.fromstring (sys.stdin.buffer.read ())

# Construct state histogram
address_states = {}

for sample in (log.find ("samples") if log.find ("samples") is not None else empty_element).iterfind ("sample"):
    threads = (sample.find ("backtrace") if sample.find ("backtrace") is not None else empty_element).iterfind ("thread")

    for thread in threads:
        running = int (thread.get ("running"))

        frame = thread.find ("frame")

        if frame is not None:
            address = frame.get ("address")

            states = address_states.setdefault (address, [0, 0])

            states[running] += 1

# Find maximal states
for address, states in list (address_states.items ()):
    n = sum (states)

    if n >= DEDUCE_MIN_N_OCCURRENCES:
        state = 0
        m     = states[0]

        for i in range (1, len (states)):
            if states[i] > m:
                state = i
                m     = states[i]

        percentage = m / n

        if percentage >= DEDUCE_MIN_PERCENTAGE:
            address_states[address] = state
        else:
            del address_states[address]
    else:
        del address_states[address]

# Replace thread states
for sample in (log.find ("samples") if log.find ("samples") is not None else empty_element).iterfind ("sample"):
    threads = (sample.find ("backtrace") if sample.find ("backtrace") is not None else empty_element).iterfind ("thread")

    for thread in threads:
        frame = thread.find ("frame")

        if frame is not None:
            address = frame.get ("address")

            running = address_states.get (address, None)

            if running is not None:
                thread.set ("running", str (running))

# Write performance log to STDOUT
sys.stdout.buffer.write (ElementTree.tostring (log))
