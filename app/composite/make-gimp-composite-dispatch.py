#!/usr/bin/env python
# -*- mode: python py-indent-offset: 2; -*-
#
# Gimp image compositing
# Copyright (C) 2003  Helvetix Victorinox, <helvetix@gimp.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

import sys
import string
import os
import ns
import pprint
import getopt
import copy

#
# This programme creates C code for gluing a collection of compositing
# functions into an array indexed by compositing function, and the
# pixel formats of its arguments.
#
# I make some assuptions about the names of the compositing functions.
#
# I look into the namespace of a set of object files and figure out
# from them what compositing functions are implemented.  This let's me
# build a table with the right cells populated with either the special
# compositing functions, or to use a generically implemented
# compositing function.


# These are in the same order as they appear in the
# ./app/base/base-enums.h GimpLayerModeEffects enumeration, because we
# (probably unwisely) use the value of the enumeration as an index
# into the Big Table.
#
# XXX I'd like some python functions that let me rummage around in C code....
#
composite_modes=[
  "GIMP_COMPOSITE_NORMAL",
  "GIMP_COMPOSITE_DISSOLVE",
  "GIMP_COMPOSITE_BEHIND",
  "GIMP_COMPOSITE_MULTIPLY",
  "GIMP_COMPOSITE_SCREEN",
  "GIMP_COMPOSITE_OVERLAY",
  "GIMP_COMPOSITE_DIFFERENCE",
  "GIMP_COMPOSITE_ADDITION",
  "GIMP_COMPOSITE_SUBTRACT",
  "GIMP_COMPOSITE_DARKEN",
  "GIMP_COMPOSITE_LIGHTEN",
  "GIMP_COMPOSITE_HUE",
  "GIMP_COMPOSITE_SATURATION",
  "GIMP_COMPOSITE_COLOR_ONLY",
  "GIMP_COMPOSITE_VALUE",
  "GIMP_COMPOSITE_DIVIDE",
  "GIMP_COMPOSITE_DODGE",
  "GIMP_COMPOSITE_BURN",
  "GIMP_COMPOSITE_HARDLIGHT",
  "GIMP_COMPOSITE_SOFTLIGHT",
  "GIMP_COMPOSITE_GRAIN_EXTRACT",
  "GIMP_COMPOSITE_GRAIN_MERGE",
  "GIMP_COMPOSITE_COLOR_ERASE",
  "GIMP_COMPOSITE_ERASE" ,
  "GIMP_COMPOSITE_REPLACE" ,
  "GIMP_COMPOSITE_ANTI_ERASE",
  "GIMP_COMPOSITE_BLEND",
  "GIMP_COMPOSITE_SHADE",
  "GIMP_COMPOSITE_SWAP",
  "GIMP_COMPOSITE_SCALE",
  "GIMP_COMPOSITE_CONVERT",
  ]

pixel_format=[
  "GIMP_PIXELFORMAT_V8",
  "GIMP_PIXELFORMAT_VA8",
  "GIMP_PIXELFORMAT_RGB8",
  "GIMP_PIXELFORMAT_RGBA8",
#  "GIMP_PIXELFORMAT_V16",
#  "GIMP_PIXELFORMAT_VA16",
#  "GIMP_PIXELFORMAT_RGB16",
#  "GIMP_PIXELFORMAT_RGBA16"
#  "GIMP_PIXELFORMAT_V32",
#  "GIMP_PIXELFORMAT_VA32",
#  "GIMP_PIXELFORMAT_RGB32",
#  "GIMP_PIXELFORMAT_RGBA32"
  "GIMP_PIXELFORMAT_ANY",
  ]


def pixel_depth_name(pixel_format):
  s = string.replace(pixel_format.lower(), "gimp_pixelformat_", "")
  return (s)

pp = pprint.PrettyPrinter(indent=4)


def functionnameify(filename):
  f = os.path.basename(filename)
  f = string.replace(f, ".o", "")
  f = string.replace(f, ".c", "")
  f = string.replace(f, ".h", "")
  f = string.replace(f, "-", "_")
  return (f)

def print_function_table(filename, function_table):

  function_table_declarations = dict()

  function_table_keys = function_table.keys()
  function_table_keys.sort()
  
  for key in function_table_keys:
    if not function_table_declarations.has_key(function_table[key][0]):
      print 'GimpCompositeFunction %s();' % (function_table[key][0])
      function_table_declarations[function_table[key][0]] = function_table[key][0]
      pass
    pass

  print ''
  print 'GimpCompositeFunction (*%s[%s][%s][%s][%s])() = {' % (functionnameify(filename),
                                                               "GIMP_COMPOSITE_N",
                                                               "GIMP_PIXELFORMAT_N",
                                                               "GIMP_PIXELFORMAT_N",
                                                               "GIMP_PIXELFORMAT_N")
  for mode in composite_modes:
    print ' { /* %s */' % (mode)
    for A in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
      print '  { /* A = %s */' % (pixel_depth_name(A))
      for B in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
        print '   /* %-6s */ {' % (pixel_depth_name(B)),
        for D in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
          key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
          if function_table.has_key(key):
            print '%s, ' % (function_table[key][0]),
          else:
            print '%s, ' % ("NULL"),
            pass
          pass
        print '},'
        pass
      print '  },'
      pass
    print ' },'
    pass

  print '};\n'
  
  return
  
def print_function_table_name(filename, function_table):

  print ''
  print 'char *%s_name[%s][%s][%s][%s] = {' % (functionnameify(filename),
                                               "GIMP_COMPOSITE_N",
                                               "GIMP_PIXELFORMAT_N",
                                               "GIMP_PIXELFORMAT_N",
                                               "GIMP_PIXELFORMAT_N")
  for mode in composite_modes:
    print ' { /* %s */' % (mode)
    for A in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
      print '  { /* A = %s */' % (pixel_depth_name(A))
      for B in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
        print '   /* %-6s */ {' % (pixel_depth_name(B)),
        for D in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
          key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
          if function_table.has_key(key):
            print '"%s", ' % (function_table[key][0]),
          else:
            print '"%s", ' % (""),
            pass
          pass
        print '},'
        pass
      print '  },'
      pass
    print ' },'
    pass

  print '};\n'
  
  return
  
def load_function_table(filename):
  nmx = ns.nmx(filename)

  gimp_composite_function = dict()

  for mode in composite_modes:
    for A in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
      for B in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
        for D in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
          key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
            
          for a in ["GIMP_PIXELFORMAT_ANY", A]:
            for b in ["GIMP_PIXELFORMAT_ANY", B]:
              for d in ["GIMP_PIXELFORMAT_ANY", D]:
                key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(a), pixel_depth_name(b), pixel_depth_name(d))
                  
                f = nmx.exports_re(key + ".*")
                if f != None: gimp_composite_function["%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))] =  [f]
                pass
              pass
            pass
          pass
        pass
      pass
    pass

  return (gimp_composite_function)


def merge_function_tables(tables):
  main_table = copy.deepcopy(tables[0][1])
  
  for t in tables[1:]:
    print >>sys.stderr, t[0]
    for mode in composite_modes:
      for A in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
        for B in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
          for D in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
            key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
            if t[1].has_key(key):
              print >>sys.stderr, "%s = %s::%s" % (key, t[0], t[1][key])
              main_table[key] = t[1][key]
              pass
            pass
          pass
        pass
      pass
    pass
            
  return (main_table)


def print_test_code(tables):
  return


def main(argv):

  objects = map(ns.nmx, argv)

  objs = objects
  objs.reverse()
  
  gimp_composite_function = dict()
  for o in objs:
    for mode in composite_modes:
      for A in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
        for B in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
          for D in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
            key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
            
            for a in [A, "GIMP_PIXELFORMAT_ANY"]:
              for b in [B, "GIMP_PIXELFORMAT_ANY"]:
                for d in [D, "GIMP_PIXELFORMAT_ANY"]:
                  composite_function = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(a), pixel_depth_name(b), pixel_depth_name(d))
                  
                  f = o.exports_re(composite_function + ".*")
                  if f != None:
                    gimp_composite_function.update({key : [f, mode, A, B, D]})
                    break
                  pass
                if gimp_composite_function.has_key(key):
                  break;
                pass
              if gimp_composite_function.has_key(key):
                break;
              pass

            if not gimp_composite_function.has_key(key):
              gimp_composite_function.update({key : ["gimp_composite_unsupported", mode, A, B, D]})
              pass

            pass
          pass
        pass
      pass
    pass


  print '/* THIS FILE IS AUTOMATICALLY GENERATED.  DO NOT EDIT */'
  print '$Id$'
  print '#include "gimp-composite.h"'
  print 'extern GimpCompositeFunction %s();' % ("gimp_composite_unsupported")
  done = dict()
  for k in gimp_composite_function.keys():
    f = gimp_composite_function[k]
    if not done.has_key(f[0]):
      print 'extern GimpCompositeFunction %s();' % (f[0])
      done.update({f[0] : None})
      pass
    pass

  if 1:
    print 'char *gimp_composite_function_name[%d][%d][%d][%d] = {' % (len(composite_modes), len(pixel_format)-1, len(pixel_format)-1, len(pixel_format)-1)
    for mode in composite_modes:
      print ' {'
      for A in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
        print '  {'
        for B in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
          print '    {',
          for D in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
            key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
            if gimp_composite_function.has_key(key):
              print '"%s", ' % (gimp_composite_function[key][0]),
            else:
              print '"%s", ' % ("gimp_composite_unsupported"),
              pass
            pass
          print '},'
          pass
        print '  },'
      
        pass
      print ' },'
      pass

    print '};'
    pass


  print ''
  print 'void (*gimp_composite_function[%d][%d][%d][%d])() = {' % (len(composite_modes), len(pixel_format)-1, len(pixel_format)-1, len(pixel_format)-1)
  for mode in composite_modes:
    print ' { /* %s */' % (mode)
    for A in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
      print '  { /* A = %s */' % (pixel_depth_name(A))
      for B in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
        print '   /* %s */ {' % (pixel_depth_name(B)),
        for D in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
          key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
          if gimp_composite_function.has_key(key):
            print '%s, ' % (gimp_composite_function[key][0]),
          else:
            print '%s, ' % ("gimp_composite_unsupported"),
            pass
          pass
        print '},'
        pass
      print '  },'
      
      pass
    print ' },'
    pass

  print '};'

  pass

def gimp_composite_regression(function_tables):

  print 'void'
  print 'gimp_composite_regression()'
  print '{'
  print '  GimpCompositeContext generic_ctx;'
  print '  GimpCompositeContext special_ctx;'

  generic_table = function_tables[0][1]
  
  for mode in composite_modes:
    for A in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
      for B in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
        for D in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
          for f in function_tables[1:]:
            key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
            if f[1].has_key(key):
              print ''
              print '  special_ctx.op = %s;' % (mode)
              print '  generic_ctx.op = %s;' % (mode)
              print '  %s(&special_ctx);' % (f[1][key][0])
              print '  %s(&generic_ctx);' % (generic_table[key][0])
              print '  if (gimp_composite_regression_compare(&generic_ctx, &special_ctx)) {'
              print '    printf("%s disagrees with %s\\n");' % (f[1][key][0], generic_table[key][0])
              print '  }'
              pass
            pass
          pass
        pass
      pass
    pass
  
  print '}'


def gimp_composite_init(function_tables):
  for o in function_tables:
    print 'extern void %s_init();' % (functionnameify(o[0]))
    pass

  print ''
  print 'void'
  print 'gimp_composite_init()'
  print '{'
  print '  char *p;'
  print ''
  print '  if ((p = getenv("GIMP_COMPOSITE"))) {'
  print '    gimp_composite_options.use = atol(p);'
  print '    g_printerr("Using new image composite functions\n");'
  print '  }'
  print ''

  print '  if (!gimp_composite_options.initialised) {'

  for o in function_tables:
    print '    %s_init();' % (functionnameify(o[0]))
    pass
  
  print '    gimp_composite_options.initialised = 1;'
  print '  }'
  print '}'
  pass

def gimp_composite_hfile(fpout):
  print >>fpout, '/* THIS FILE IS AUTOMATICALLY GENERATED.  DO NOT EDIT */'
  print >>fpout, ''
  print >>fpout, 'typedef void (*GimpCompositeFunction)(GimpCompositeContext *);'
  print >>fpout, 'typedef GimpCompositeFunction (*GimpCompositeFunctionTable[%s][%s][%s][%s]);' % ("GIMP_COMPOSITE_N",
                                                                                                   "GIMP_PIXELFORMAT_N",
                                                                                                   "GIMP_PIXELFORMAT_N",
                                                                                                   "GIMP_PIXELFORMAT_N")
  pass

gimp_composite_hfile(open("gimp-composite-dispatch.h", "w"))


print '/* THIS FILE IS AUTOMATICALLY GENERATED.  DO NOT EDIT */'
print '#include "config.h"'
print '#include <glib-object.h>'
print '#include <stdlib.h>'
print '#include "base/base-types.h"'
print '#include "gimp-composite.h"'
print '#include "gimp-composite-dispatch.h"'
print 'extern GimpCompositeFunction %s();' % ("gimp_composite_unsupported")
print ''


d = list()
for f in sys.argv[1:]:
  dd = load_function_table(f)
  d.append((f, dd))
  print_function_table(f, dd)
  pass

main_table = merge_function_tables(d)

print_function_table("gimp_composite_function", main_table)
print_function_table_name("gimp_composite_function", main_table)

gimp_composite_init(d)

#gimp_composite_regression(d)

sys.exit(0)
