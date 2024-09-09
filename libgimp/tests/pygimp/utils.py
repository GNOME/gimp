#!/usr/bin/env python3

import gi
from gi.repository import Gimp, Gio

import inspect
import re
import os
import sys

gimp_test_filename = ''

def gimp_assert(subtest_name, test, image=None, outpath=None):
  '''
  Please call me like this, for instance, if I were testing if gimp_image_new()
  succeeded:
    gimp_assert("gimp_image_new()", image is not None)
  '''
  if not test:
    frames = inspect.getouterframes(inspect.currentframe())
    sys.stderr.write("\n**** START FAILED SUBTEST *****\n")
    sys.stderr.write("ERROR: {} - line {}: {}\n".format(gimp_test_filename,
                                                        frames[1].lineno,
                                                        subtest_name))
    if image is not None and outpath is not None:
      Gimp.file_save(Gimp.RunMode.NONINTERACTIVE, image,
                     Gio.file_new_for_path(outpath),
                     None)
      sys.stderr.write("       Test file saved as: {}\n".format(outpath))

    sys.stderr.write("***** END FAILED SUBTEST ******\n\n")
  assert test

def gimp_c_assert(c_filename, error_msg, test):
  '''
  This is called by the platform only, and print out the GError message from the
  C test plug-in.
  '''
  if not test:
    sys.stderr.write("\n**** START FAILED SUBTEST *****\n")
    sys.stderr.write("ERROR: {}: {}\n".format(c_filename, error_msg))
    sys.stderr.write("***** END FAILED SUBTEST ******\n\n")
  assert test
