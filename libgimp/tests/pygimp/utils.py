#!/usr/bin/env python3

import gi
from gi.repository import Babl, Gegl, Gimp, Gio

import inspect
import re
import os
import sys

gimp_test_filename = ''

def gimp_assert(subtest_name, test, image=None, outpath=None, cmp_data_path=None):
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

      if cmp_data_path is not None:
        if not os.path.isfile(cmp_data_path):
          sys.stderr.write("       To get statistical comparison, store reference data in: {}\n".format(cmp_data_path))
        else:
          img_width = image.get_width()
          img_height = image.get_height()

          proc   = Gimp.get_pdb().lookup_procedure('file-raw-load')
          config = proc.create_config()
          config.set_property('run-mode', Gimp.RunMode.NONINTERACTIVE)
          config.set_property('file', Gio.file_new_for_path(cmp_data_path))
          config.set_property('width', img_width)
          config.set_property('height', img_height)
          config.set_property('pixel-format', 'rgba-8bpc')
          result = proc.run(config)
          status = result.index(0)

          if status == Gimp.PDBStatusType.SUCCESS:
            cmp_img = result.index(1)

            layer = image.merge_visible_layers(Gimp.MergeType.CLIP_TO_IMAGE)
            buffer = layer.get_buffer()
            rect = buffer.get_extent()
            data = buffer.get(rect, 1.0, "R'G'B'A u8", Gegl.AbyssPolicy.BLACK)

            cmp_buffer = cmp_img.get_layers()[0].get_buffer()
            cmp_data = cmp_buffer.get(rect, 1.0, "R'G'B'A u8", Gegl.AbyssPolicy.BLACK)

            sys.stderr.write("       Comparison with {}:\n".format(cmp_data_path))

            n_diff_pixels            = 0
            n_perceptually_identical = 0
            n_one_off                = 0
            num_samples              = 4
            num_identical_samples    = 4
            num_one_off_samples      = 4
            for x in range(img_width):
              for y in range(img_height):
                pos = (x + y * img_width) * 4
                r1 = data[pos]
                r2 = cmp_data[pos]
                g1 = data[pos + 1]
                g2 = cmp_data[pos + 1]
                b1 = data[pos + 2]
                b2 = cmp_data[pos + 2]
                a1 = data[pos + 3]
                a2 = cmp_data[pos + 3]
                if r1 != r2 or g1 != g2 or b1 != b2 or a1 != a2:
                  c1 = Gegl.Color.new("black")
                  c1.set_rgba_with_space(r1/255, g1/255, b1/255, a1/255, Babl.space("sRGB"))
                  c2 = Gegl.Color.new("black")
                  c2.set_rgba_with_space(r2/256, g2/255, b2/255, a2/255, Babl.space("sRGB"))
                  n_diff_pixels += 1
                  if Gimp.color_is_perceptually_identical(c1, c2):
                    if num_identical_samples > 0:
                      sys.stderr.write("       - Example of differing pixel (yet perceptually identical) at {}x{}: ({}, {}, {}, {}) vs. ({}, {}, {}, {})\n".format(x, y, r1, g1, b1, a1, r2, g2, b2, a2))
                      num_identical_samples -= 1
                    n_perceptually_identical += 1
                  elif abs(r1 - r2) < 2 and abs(g1 - g2) < 2 and abs(b1 - b2) < 2 and abs(a1 - a2) < 2:
                    if num_one_off_samples > 0:
                      sys.stderr.write("       - Example of differing pixel (off-by-one) at {}x{}: ({}, {}, {}, {}) vs. ({}, {}, {}, {})\n".format(x, y, r1, g1, b1, a1, r2, g2, b2, a2))
                      num_one_off_samples -= 1
                    n_one_off += 1
                  else:
                    if num_samples > 0:
                      sys.stderr.write("       - Example of differing pixel at {}x{}: ({}, {}, {}, {}) vs. ({}, {}, {}, {})\n".format(x, y, r1, g1, b1, a1, r2, g2, b2, a2))
                      num_samples -= 1
            sys.stderr.write("       - {} pixels are different\n".format(n_diff_pixels))
            sys.stderr.write("       - Among them {} different pixels are perceptually identical\n".format(n_perceptually_identical))
            sys.stderr.write("       - Among them {} different pixels are off by one\n".format(n_one_off))
            if n_diff_pixels == n_perceptually_identical + n_one_off:
              sys.stderr.write("       - All different pixels are either perceptually identical or off by one\n")

            cmp_img.delete()
          else:
            sys.stderr.write("       Failed to load for statistical comparison: {}\n".format(cmp_data_path))

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
