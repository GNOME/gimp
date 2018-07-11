#!/usr/bin/env python2

"""
defcheck.py -- Consistency check for the .def files.
Copyright (C) 2006  Simon Budig <simon@gimp.org>

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


This is a hack to check the consistency of the .def files compared to
the respective libraries.

Invoke in the top level of the gimp source tree after compiling GIMP.
If srcdir != builddir, run it in the build directory and pass the name
of the source directory on the command-line.

Needs the tool "nm" to work

"""

import sys, commands

from os import path

def_files = (
   "libgimpbase/gimpbase.def",
   "libgimpcolor/gimpcolor.def",
   "libgimpconfig/gimpconfig.def",
   "libgimp/gimp.def",
   "libgimp/gimpui.def",
   "libgimpmath/gimpmath.def",
   "libgimpmodule/gimpmodule.def",
   "libgimpthumb/gimpthumb.def",
   "libgimpwidgets/gimpwidgets.def"
)

have_errors = 0

srcdir = None
if len(sys.argv) > 1:
   srcdir = sys.argv[1]
   if not path.exists(srcdir):
      print "Directory '%s' does not exist" % srcdir
      sys.exit (-1)

for df in def_files:
   directory, name = path.split (df)
   basename, extension = name.split (".")
   libname = path.join(directory, ".libs", "lib" + basename + "-*.so")

   filename = df
   if srcdir:
      filename = path.join(srcdir, df)
   try:
      defsymbols = file (filename).read ().split ()[1:]
   except IOError, message:
      print message
      if not srcdir:
         print "You should run this script from the toplevel source directory."
      sys.exit (-1)

   doublesymbols = []
   for i in range (len (defsymbols)-1, 0, -1):
      if defsymbols[i] in defsymbols[:i]:
         doublesymbols.append ((defsymbols[i], i+2))

   unsortindex = -1
   for i in range (len (defsymbols)-1):
      if defsymbols[i] > defsymbols[i+1]:
         unsortindex = i+1
         break;

   status, nm = commands.getstatusoutput ("nm --defined-only --extern-only " +
                                          libname)
   if status != 0:
      print "trouble reading %s - has it been compiled?" % libname
      continue

   nmsymbols = nm.split()[2::3]
   nmsymbols = [s for s in nmsymbols if s[0] != '_']

   missing_defs = [s for s in nmsymbols  if s not in defsymbols]
   missing_nms  = [s for s in defsymbols if s not in nmsymbols]

   if unsortindex >= 0 or missing_defs or missing_nms or doublesymbols:
      print
      print "Problem found in", filename

      if missing_defs:
         print "  the following symbols are in the library,"
         print "  but are not listed in the .def-file:"
         for s in missing_defs:
            print "     +", s
         print

      if missing_nms:
         print "  the following symbols are listed in the .def-file,"
         print "  but are not exported by the library."
         for s in missing_nms:
            print "     -", s
         print

      if doublesymbols:
         print "  the following symbols are listed multiple times in the .def-file,"
         for s in doublesymbols:
            print "     : %s (line %d)" % s
         print

      if unsortindex >= 0:
         print "  the .def-file is not properly sorted (line %d)" % (unsortindex + 2)
         print

      have_errors = -1

sys.exit (have_errors)
