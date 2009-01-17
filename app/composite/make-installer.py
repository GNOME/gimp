#!/usr/bin/env python
#
# Gimp image compositing
# Copyright (C) 2003  Helvetix Victorinox, <helvetix@gimp.org>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import sys
import string
import os
import ns
import pprint
import optparse
import copy
import re

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
  "GIMP_COMPOSITE_XOR",
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


def mode_name(mode):
  s = string.replace(mode.lower(), "gimp_composite_", "")
  return (s)

def pixel_depth_name(pixel_format):
  s = string.replace(pixel_format.lower(), "gimp_pixelformat_", "")
  return (s)


pp = pprint.PrettyPrinter(indent=4)


def sanitize_filename(filename):
  return re.sub('^lib[^-]+-', '', filename)

def functionnameify(filename):
  f = os.path.basename(filename)
  f = sanitize_filename(f)
  f = string.replace(f, ".o", "")
  f = string.replace(f, ".c", "")
  f = string.replace(f, ".h", "")
  f = string.replace(f, "-", "_")
  return (f)

def filenameify(filename):
  f = os.path.basename(filename)
  f = sanitize_filename(f)
  f = string.replace(f, ".o", "")
  f = string.replace(f, ".c", "")
  f = string.replace(f, ".h", "")
  return (f)

def print_function_table(fpout, name, function_table, requirements=[]):

  if len(function_table) < 1:
    return;

  print >>fpout, 'static const struct install_table {'
  print >>fpout, '  GimpCompositeOperation mode;'
  print >>fpout, '  GimpPixelFormat A;'
  print >>fpout, '  GimpPixelFormat B;'
  print >>fpout, '  GimpPixelFormat D;'
  print >>fpout, '  void (*function)(GimpCompositeContext *);'
  #print >>fpout, '  char *name;'
  print >>fpout, '} _%s[] = {' % (functionnameify(name))

  for r in requirements:
    print >>fpout, '#if %s' % (r)
    pass

  for mode in composite_modes:
    for A in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
      for B in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
        for D in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
          key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
          if function_table.has_key(key):
            print >>fpout, ' { %s, %s, %s, %s, %s },' % (mode, A, B, D, function_table[key][0])
            pass
          pass
        pass
      pass
    pass

  for r in requirements:
    print >>fpout, '#endif'
    pass

  print >>fpout, ' { 0, 0, 0, 0, NULL }'
  print >>fpout, '};'

  return

def print_function_table_name(fpout, name, function_table):

  print >>fpout, ''
  print >>fpout, 'char *%s_name[%s][%s][%s][%s] = {' % (functionnameify(name), "GIMP_COMPOSITE_N", "GIMP_PIXELFORMAT_N", "GIMP_PIXELFORMAT_N", "GIMP_PIXELFORMAT_N")
  for mode in composite_modes:
    print >>fpout, ' { /* %s */' % (mode)
    for A in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
      print >>fpout, '  { /* A = %s */' % (pixel_depth_name(A))
      for B in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
        print >>fpout, '   /* %-6s */ {' % (pixel_depth_name(B)),
        for D in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
          key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
          if function_table.has_key(key):
            print >>fpout, '"%s", ' % (function_table[key][0]),
          else:
            print >>fpout, '"%s", ' % (""),
            pass
          pass
        print >>fpout, '},'
        pass
      print >>fpout, '  },'
      pass
    print >>fpout, ' },'
    pass

  print >>fpout, '};\n'

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
    #print >>sys.stderr, t[0]
    for mode in composite_modes:
      for A in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
        for B in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
          for D in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
            key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
            if t[1].has_key(key):
              #print >>sys.stderr, "%s = %s::%s" % (key, t[0], t[1][key])
              main_table[key] = t[1][key]
              pass
            pass
          pass
        pass
      pass
    pass

  return (main_table)


def gimp_composite_regression(fpout, function_tables, options):

  # XXX move all this out to C code, instead of here.
  print >>fpout, '#include "config.h"'
  print >>fpout, ''
  print >>fpout, '#include <stdio.h>'
  print >>fpout, '#include <stdlib.h>'
  print >>fpout, '#include <string.h>'
  print >>fpout, ''
  print >>fpout, '#include <sys/time.h>'
  print >>fpout, ''
  print >>fpout, '#include <glib-object.h>'
  print >>fpout, ''
  print >>fpout, '#include "base/base-types.h"'
  print >>fpout, ''
  print >>fpout, '#include "gimp-composite.h"'

  print >>fpout, '#include "gimp-composite-regression.h"'
  print >>fpout, '#include "gimp-composite-util.h"'
  print >>fpout, '#include "gimp-composite-generic.h"'
  print >>fpout, '#include "%s.h"' % (filenameify(options.file))
  print >>fpout, ''
  print >>fpout, 'int'
  print >>fpout, '%s_test (int iterations, int n_pixels)' % (functionnameify(options.file))
  print >>fpout, '{'

  for r in options.requires:
    print >>fpout, '#if %s' % (r)
    pass

  print >>fpout, '  GimpCompositeContext generic_ctx;'
  print >>fpout, '  GimpCompositeContext special_ctx;'
  print >>fpout, '  double ft0;'
  print >>fpout, '  double ft1;'
  print >>fpout, '  gimp_rgba8_t *rgba8D1;'
  print >>fpout, '  gimp_rgba8_t *rgba8D2;'
  print >>fpout, '  gimp_rgba8_t *rgba8A;'
  print >>fpout, '  gimp_rgba8_t *rgba8B;'
  print >>fpout, '  gimp_rgba8_t *rgba8M;'
  print >>fpout, '  gimp_va8_t *va8A;'
  print >>fpout, '  gimp_va8_t *va8B;'
  print >>fpout, '  gimp_va8_t *va8M;'
  print >>fpout, '  gimp_va8_t *va8D1;'
  print >>fpout, '  gimp_va8_t *va8D2;'
  print >>fpout, '  int i;'
  print >>fpout, ''

  print >>fpout, '  printf("\\nRunning %s tests...\\n");' % (functionnameify(options.file))
  print >>fpout, '  if (%s_init () == 0)' % (functionnameify(options.file))
  print >>fpout, '    {'
  print >>fpout, '      printf("%s: Instruction set is not available.\\n");' % (functionnameify(options.file))
  print >>fpout, '      return (0);'
  print >>fpout, '    }'


  print >>fpout, ''
  print >>fpout, '  rgba8A =  gimp_composite_regression_random_rgba8(n_pixels+1);'
  print >>fpout, '  rgba8B =  gimp_composite_regression_random_rgba8(n_pixels+1);'
  print >>fpout, '  rgba8M =  gimp_composite_regression_random_rgba8(n_pixels+1);'
  print >>fpout, '  rgba8D1 = (gimp_rgba8_t *) calloc(sizeof(gimp_rgba8_t), n_pixels+1);'
  print >>fpout, '  rgba8D2 = (gimp_rgba8_t *) calloc(sizeof(gimp_rgba8_t), n_pixels+1);'
  print >>fpout, '  va8A =    (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);'
  print >>fpout, '  va8B =    (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);'
  print >>fpout, '  va8M =    (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);'
  print >>fpout, '  va8D1 =   (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);'
  print >>fpout, '  va8D2 =   (gimp_va8_t *)   calloc(sizeof(gimp_va8_t), n_pixels+1);'
  print >>fpout, ''
  print >>fpout, '  for (i = 0; i < n_pixels; i++)'
  print >>fpout, '    {'
  print >>fpout, '      va8A[i].v = i;'
  print >>fpout, '      va8A[i].a = 255-i;'
  print >>fpout, '      va8B[i].v = i;'
  print >>fpout, '      va8B[i].a = i;'
  print >>fpout, '      va8M[i].v = i;'
  print >>fpout, '      va8M[i].a = i;'
  print >>fpout, '    }'
  print >>fpout, ''

  #pp.pprint(function_tables)

  generic_table = function_tables

  composite_modes.sort();

  for mode in composite_modes:
    for A in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
      for B in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
        for D in filter(lambda pf: pf != "GIMP_PIXELFORMAT_ANY", pixel_format):
          key = "%s_%s_%s_%s" % (string.lower(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
          if function_tables.has_key(key):
            #print key
            print >>fpout, ''
            print >>fpout, '  gimp_composite_context_init (&special_ctx, %s, %s, %s, %s, %s, n_pixels, (unsigned char *) %sA, (unsigned char *) %sB, (unsigned char *) %sB, (unsigned char *) %sD2);' % (
              mode, A, B, D, D, pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D), pixel_depth_name(D))

            print >>fpout, '  gimp_composite_context_init (&generic_ctx, %s, %s, %s, %s, %s, n_pixels, (unsigned char *) %sA, (unsigned char *) %sB, (unsigned char *) %sB, (unsigned char *) %sD1);' % (
              mode, A, B, D, D, pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D), pixel_depth_name(D))

            print >>fpout, '  ft0 = gimp_composite_regression_time_function (iterations, %s, &generic_ctx);' % ("gimp_composite_dispatch")
            print >>fpout, '  ft1 = gimp_composite_regression_time_function (iterations, %s, &special_ctx);' % (generic_table[key][0])
            print >>fpout, '  if (gimp_composite_regression_compare_contexts ("%s", &generic_ctx, &special_ctx))' % (mode_name(mode))

            print >>fpout, '    {'
            print >>fpout, '      printf("%s_%s_%s_%s failed\\n");' % (mode_name(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
            print >>fpout, '      return (1);'
            print >>fpout, '    }'
            print >>fpout, '  gimp_composite_regression_timer_report ("%s_%s_%s_%s", ft0, ft1);' % (mode_name(mode), pixel_depth_name(A), pixel_depth_name(B), pixel_depth_name(D))
            pass
          pass
        pass
      pass
    pass

  for r in options.requires:
    print >>fpout, '#endif'
    pass

  print >>fpout, '  return (0);'
  print >>fpout, '}'

  print >>fpout, ''
  print >>fpout, 'int'
  print >>fpout, 'main (int argc, char *argv[])'
  print >>fpout, '{'
  print >>fpout, '  int iterations;'
  print >>fpout, '  int n_pixels;'
  print >>fpout, ''
  print >>fpout, '  srand (314159);'
  print >>fpout, ''
  print >>fpout, '  putenv ("GIMP_COMPOSITE=0x1");'
  print >>fpout, ''
  print >>fpout, '  iterations = %d;' % options.iterations
  print >>fpout, '  n_pixels = %d;' % options.n_pixels
  print >>fpout, ''
  print >>fpout, '  argv++, argc--;'
  print >>fpout, '  while (argc >= 2)'
  print >>fpout, '    {'
  print >>fpout, '      if (argc > 1 && (strcmp (argv[0], "--iterations") == 0 || strcmp (argv[0], "-i") == 0))'
  print >>fpout, '        {'
  print >>fpout, '          iterations = atoi(argv[1]);'
  print >>fpout, '          argc -= 2, argv++; argv++;'
  print >>fpout, '        }'
  print >>fpout, '      else if (argc > 1 && (strcmp (argv[0], "--n-pixels") == 0 || strcmp (argv[0], "-n") == 0))'
  print >>fpout, '        {'
  print >>fpout, '          n_pixels = atoi (argv[1]);'
  print >>fpout, '          argc -= 2, argv++; argv++;'
  print >>fpout, '        }'
  print >>fpout, '      else'
  print >>fpout, '        {'
  print >>fpout, '          printf("Usage: gimp-composites-*-test [-i|--iterations n] [-n|--n-pixels n]");'
  print >>fpout, '          exit(1);'
  print >>fpout, '        }'
  print >>fpout, '    }'
  print >>fpout, ''
  print >>fpout, '  gimp_composite_generic_install ();'
  print >>fpout, ''
  print >>fpout, '  return (%s_test (iterations, n_pixels));' % (functionnameify(options.file))
  print >>fpout, '}'

  return


def gimp_composite_installer_install2(fpout, name, function_table, requirements=[]):
  print >>fpout, ''
  print >>fpout, 'gboolean'
  print >>fpout, '%s_install (void)' % (functionnameify(name))
  print >>fpout, '{'

  if len(function_table) >= 1:
    print >>fpout, '  static const struct install_table *t = _%s;' % (functionnameify(name))
    print >>fpout, ''
    print >>fpout, '  if (%s_init ())' % functionnameify(name)
    print >>fpout, '    {'
    print >>fpout, '      for (t = &_%s[0]; t->function != NULL; t++)' % (functionnameify(name))
    print >>fpout, '        {'
    print >>fpout, '          gimp_composite_function[t->mode][t->A][t->B][t->D] = t->function;'
    print >>fpout, '        }'
    print >>fpout, '      return (TRUE);'
    print >>fpout, '    }'
  else:
    print >>fpout, '  /* nothing to do */'
    pass

  print >>fpout, ''
  print >>fpout, '  return (FALSE);'
  print >>fpout, '}'
  pass

def gimp_composite_installer_install3(fpout, name, requirements=[], cpu_feature=[]):
  if not requirements and not cpu_feature:
    return

  print >>fpout, ''
  print >>fpout, 'gboolean'
  print >>fpout, '%s_init (void)' % (functionnameify(name))
  print >>fpout, '{'

  need_endif = False

  for r in requirements:
    print >>fpout, '#if %s' % (r)
    pass

  if cpu_feature:
    if len(cpu_feature) >= 2:
      features = []
      for f in cpu_feature:
        features.append('cpu & GIMP_CPU_ACCEL_%s' % f)
      feature_test = ' || '.join(features)

      print >>fpout, '  GimpCpuAccelFlags cpu = gimp_cpu_accel_get_support ();'
      print >>fpout, ''
    else:
      feature_test = 'gimp_cpu_accel_get_support () & GIMP_CPU_ACCEL_%s' % cpu_feature[0]

    print >>fpout, '  if (%s)' % feature_test
    print >>fpout, '    {'
    print >>fpout, '      return (TRUE);'
    print >>fpout, '    }'

    if requirements:
      print >>fpout, '#endif'
      print >>fpout, ''
  else:
    print >>fpout, '  return (TRUE);'

    if requirements:
      print >>fpout, '#else'
      need_endif = True

  if requirements or cpu_feature:
    print >>fpout, '  return (FALSE);'

  if need_endif:
    print >>fpout, '#endif'

  print >>fpout, '}'
  pass


def gimp_composite_hfile(fpout, name, function_table):
  print >>fpout, '/* THIS FILE IS AUTOMATICALLY GENERATED.  DO NOT EDIT */'
  print >>fpout, '/* REGENERATE BY USING make-installer.py */'
  print >>fpout, ''
  print >>fpout, 'void %s_install (void);' % (functionnameify(name))
  print >>fpout, ''
  print >>fpout, 'typedef void (*%s_table[%s][%s][%s][%s]);' % (functionnameify(name), "GIMP_COMPOSITE_N", "GIMP_PIXELFORMAT_N", "GIMP_PIXELFORMAT_N", "GIMP_PIXELFORMAT_N")

  return

def gimp_composite_cfile(fpout, name, function_table, requirements=[], cpu_feature=[]):
  print >>fpout, '/* THIS FILE IS AUTOMATICALLY GENERATED.  DO NOT EDIT */'
  print >>fpout, '/* REGENERATE BY USING make-installer.py */'
  print >>fpout, '#include "config.h"'
  print >>fpout, '#include <stdlib.h>'
  print >>fpout, '#include <stdio.h>'
  print >>fpout, '#include <glib-object.h>'
  if cpu_feature:
    print >>fpout, '#include "libgimpbase/gimpbase.h"'
  print >>fpout, '#include "base/base-types.h"'
  print >>fpout, '#include "gimp-composite.h"'
  print >>fpout, ''
  print >>fpout, '#include "%s.h"' % (filenameify(name))
  print >>fpout, ''

  print_function_table(fpout, name, function_table, requirements)

  gimp_composite_installer_install2(fpout, name, function_table, requirements)

  gimp_composite_installer_install3(fpout, name, requirements, cpu_feature)

  return

###########################################

op = optparse.OptionParser(version="$Revision$")
op.add_option('-f', '--file', action='store',         type='string', dest='file',        default=None,
              help='the input object file')
op.add_option('-t', '--test', action='store_true',                   dest='test',        default=False,
              help='generate regression testing code')
op.add_option('-i', '--iterations', action='store',   type='int',    dest='iterations',  default=10,
              help='number of iterations in regression tests')
op.add_option('-n', '--n-pixels', action='store',     type="int",    dest='n_pixels',    default=1024*8192+16+1,
              help='number of pixels in each regression test iteration')
op.add_option('-r', '--requires', action='append',    type='string', dest='requires',    default=[],
              help='cpp #if conditionals')
op.add_option('-c', '--cpu-feature', action='append', type='string', dest='cpu_feature', default=[],
              help='cpu_accel feature tests')

options, args = op.parse_args()

table = load_function_table(options.file)

gimp_composite_cfile(open(filenameify(options.file) + "-installer.c", "w"), options.file, table, options.requires, options.cpu_feature)

if options.test == True:
  gimp_composite_regression(open(filenameify(options.file) + "-test.c", "w"), table, options)
  pass

sys.exit(0)
