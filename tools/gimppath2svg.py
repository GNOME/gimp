#!/usr/bin/env python2

import sys,re

"""
gimppath2svg.py -- Converts Gimp-Paths to a SVG-File.
Copyright (C) 2000  Simon Budig <simon@gimp.org>

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


Usage: gimppath2svg.py [infile [outfile]]
"""


svgtemplate = """<?xml version="1.0" standalone="yes"?>
<svg width="%d" height="%d">
<g>
<path id="%s" transform="translate (%d,%d)"
      style="stroke:#000000; stroke-width:1; fill:none"
      d="%s"/>
</g>
</svg>
"""

emptysvgtemplate = """<?xml version="1.0" standalone="yes"?>
<svg width="1" height="1">
</svg>
"""


class Path:
   def __init__(self):
      self.name = ""
      self.svgpath = ""
      self.gimppoints = [[]]
      self.bounds = None

   def readgimpfile (self, filedesc):
      text = filedesc.readlines()
      for line in text:
         namematch = re.match ("Name: (.*)$", line)
         if namematch:
            path.name = namematch.group(1)
         pointmatch = re.match ("TYPE: (\d) X: (\d+) Y: (\d+)", line)
         if pointmatch:
            if pointmatch.group (1) == "3":
               path.gimppoints.append ([])
            (x, y) = map (int, pointmatch.groups()[1:])
            path.gimppoints[-1].append (map (int, pointmatch.groups()))
            if self.bounds:
               if self.bounds[0] > x: self.bounds[0] = x
               if self.bounds[1] > y: self.bounds[1] = y
               if self.bounds[2] < x: self.bounds[2] = x
               if self.bounds[3] < y: self.bounds[3] = y
            else:
               self.bounds = [x,y,x,y]

   def makesvg (self):
      for path in self.gimppoints:
         if path:
            start = path[0]
            svg = "M %d %d " % tuple (start[1:])
            path = path[1:]
            while path:
               curve = path [0:3]
               path = path[3:]
               if len (curve) == 2:
                  svg = svg + "C %d %d %d %d %d %d z " % tuple (
                           tuple (curve [0][1:]) +
                           tuple (curve [1][1:]) +
                           tuple (start [1:]))
               if len (curve) == 3:
                  svg = svg + "C %d %d %d %d %d %d " % tuple (
                           tuple (curve [0][1:]) +
                           tuple (curve [1][1:]) +
                           tuple (curve [2][1:]))
            self.svgpath = self.svgpath + svg

   def writesvgfile (self, outfile):
      if self.svgpath:
         svg = svgtemplate % (self.bounds[2]-self.bounds[0],
                              self.bounds[3]-self.bounds[1],
                              self.name,
                              -self.bounds[0], -self.bounds[1],
                              self.svgpath)
      else:
         svg = emptysvgtemplate
      outfile.write (svg)


if len (sys.argv) > 1:
   infile = open (sys.argv[1])
else:
   infile = sys.stdin

if len (sys.argv) > 2:
   outfile = open (sys.argv[2], "w")
else:
   outfile = sys.stdout


path = Path()
path.readgimpfile (infile)
path.makesvg()
path.writesvgfile (outfile)
