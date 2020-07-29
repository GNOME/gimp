#!/usr/bin/env python3

"""
performance-log-close-tags.py -- Closes open tags in GIMP performance logs
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


Usage: performance-log-close-tags.py < infile > outfile
"""

from xml.etree import ElementTree
import sys

class OpenTagsTracker:
    open_tags = []

    def start (self, tag, attrib):
        self.open_tags.append (tag)

    def end (self, tag):
        self.open_tags.pop ()

# Read performance log from STDIN
text = sys.stdin.buffer.read ()

# Write performance log to STDOUT
sys.stdout.buffer.write (text)

# Track open tags
tracker = OpenTagsTracker ()

ElementTree.XMLParser (target = tracker).feed (text)

# Close remaining open tags
for tag in reversed (tracker.open_tags):
    print ("</%s>" % tag)
