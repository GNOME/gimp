#!/usr/bin/env python3

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

Invoke in the build directory and pass the name
of the built .def files on the command-line.

Needs the tool "nm", "objdump", "dumpbin" or "dyld_info" to work

"""

import os, sys, subprocess, shutil, glob
from os import getenv, path

def_files = sys.argv[1:]

have_errors = 0

libextension   = ".so"
#command        = getenv("NM", default="nm") + " --defined-only --extern-only "
command        = getenv("NM", default="nm") + " -D " #see https://gitlab.gnome.org/GNOME/gimp/-/work_items/16255
libprefix      = "lib"
platform_linux = True
platform_win32 = False
platform_macos = False
if sys.platform in ['win32', 'cygwin']:
   libextension   = ".dll"
   command        = "objdump -p "
   if shutil.which("dumpbin"):
     command      = "dumpbin /EXPORTS "
     libprefix    = ""
   platform_linux = False
   platform_win32 = True
   platform_macos = False
elif sys.platform == 'darwin':
   libextension   = ".dylib"
   command        = "dyld_info -exports "
   platform_linux = False
   platform_win32 = False
   platform_macos = True

for df in def_files:
   directory, name = path.split (df)
   basename, extension = name.split (".")
   libname = path.join(os.getcwd(), directory, libprefix + basename + "-*" + libextension)
   matches = glob.glob(libname)
   if matches:
     libname = matches[0]
   #FIXME: This leaks to ninja stdout, which should not happen
   #print ("platform: " + sys.platform + " - extracting symbols from " + libname)

   filename = df
   try:
      defsymbols = open (filename).read ().split ()[1:]
   except IOError as message:
      print(message)
      sys.exit (-1)

   doublesymbols = []
   for i in range (len (defsymbols)-1, 0, -1):
      if defsymbols[i] in defsymbols[:i]:
         doublesymbols.append ((defsymbols[i], i+2))

   sorterrors = ""
   sortok = True
   for i in range (len (defsymbols)-1):
      if defsymbols[i].lower() > defsymbols[i+1].lower():
         sorterrors += f"{defsymbols[i]} > {defsymbols[i+1]}\n"
         sortok = False
   sorterrors = sorterrors.split(sep='\n')

   status, nm = subprocess.getstatusoutput (command + f'"{libname}"')
   if status != 0:
      print("trouble reading {} - has it been compiled?".format(libname))
      print(nm)
      have_errors = -1
      continue

   nmsymbols = ""
   if platform_linux:
      #nmsymbols = nm
      lines = nm.split(sep='\n')
      for line in lines:
         parts = line.split()
         if len(parts) == 3 and parts[1].upper() in "TDBR":
            nmsymbols += " 0 0 " + parts[2].split('@')[0]
         elif len(parts) == 2 and parts[0].upper() in "TDBR":
            #If the address is omitted for certain sections
            nmsymbols += " 0 0 " + parts[1].split('@')[0]

   elif platform_win32 and not shutil.which("dumpbin"): # Windows MSYS2
      objnm = nm.split(sep='\n')
      found = False
      nmsymbols = ""
      for s in objnm:
         if "Ordinal   Hint Name" in s or " Ordinal      RVA  Name" in s:
            found = True
         elif found:
            s = s.strip()
            if not s:
               break
            nmsymbols += " 0 0 " + s.split()[-1] # Keep the [2::3] logic happy

   elif platform_win32: # Windows MSVC
      dbin = nm.split(sep='\n')
      found = False
      nmsymbols = ""
      for s in dbin:
         if "ordinal" in s and "hint" in s and "RVA" in s:
            found = True
         elif found and s.strip() and "Summary" not in s:
            parts = s.split()
            if len(parts) >= 4:
               nmsymbols += " 0 0 " + parts[3] # Keep the [2::3] logic happy

   elif platform_macos:
      lines = nm.split(sep='\n')
      for line in lines:
         parts = line.split()
         if parts and parts[0].startswith("0x"):
            symbol = parts[-1]
            if symbol.startswith('_'):
               symbol = symbol[1:]
            nmsymbols += " 0 0 " + symbol # Keep the [2::3] logic happy

   nmsymbols = nmsymbols.split()[2::3]
   nmsymbols = [s for s in nmsymbols if s[0] != '_' and not s.startswith("OBJC_")]

   missing_defs = [s for s in nmsymbols  if s not in defsymbols]
   missing_nms  = [s for s in defsymbols if s not in nmsymbols]

   if missing_defs or missing_nms or doublesymbols or not sortok:
      print()
      print("Problem found in", filename)

      if missing_defs:
         print("  the following symbols are in the library,")
         print("  but are not listed in the .def-file:")
         for s in missing_defs:
            print("     +", s)
         print()

      if missing_nms:
         print("  the following symbols are listed in the .def-file,")
         print("  but are not exported by the library.")
         for s in missing_nms:
            print("     -", s)
         print()

      if doublesymbols:
         print("  the following symbols are listed multiple times in the .def-file,")
         for s in doublesymbols:
            print("     : %s (line %d)" % s)
         print()

      if not sortok:
         print("  the .def-file is not properly sorted in the following cases")
         for s in sorterrors:
            if s != "":
               print("     * ", s)

      have_errors = -1

sys.exit (have_errors)
