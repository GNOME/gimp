#!/usr/bin/env python3

"""
generate-icon-makefiles.py -- Generate icons/(Color|Symbolic)/icon-list.mk
Copyright (C) 2022 Jehan

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


Usage: generate-icon-makefiles.py
"""

import os.path

tools_dir = os.path.dirname(os.path.realpath(__file__))
icons_dir = os.path.join(tools_dir, '../icons')

list_dir  = os.path.join(icons_dir, 'icon-lists')

color_mk    = os.path.join(icons_dir, 'Color', 'icon-list.mk')
symbolic_mk = os.path.join(icons_dir, 'Symbolic', 'icon-list.mk')

def print_icons(indir, filenames, max_len, prefix, suffix, outfile, endlist=True):
  icons = []
  for filename in filenames:
    icon_list = os.path.join(indir, filename)
    with open(icon_list, mode='r') as f:
      icons += [line.strip() for line in f]

  if max_len is None:
    max_len = len(max(icons, key=len)) + len(prefix + suffix)

  # Using tabs, displayed as 8 chars in our coding style. Computing
  # needed tabs for proper alignment.
  needed_tabs = int(max_len / 8) + (1 if max_len % 8 != 0 else 0)
  for icon in icons[:-1]:
    icon_path = prefix + icon + suffix
    tab_mult = needed_tabs - int(len(icon_path) / 8) + 1
    icon_path = "\t{}{}\\".format(icon_path, "\t" * tab_mult)
    print(icon_path, file=outfile)
  else:
    if endlist:
      icon_path = "\t{}".format(prefix) + icons[-1] + suffix
      print(icon_path, file=outfile)
    else:
      icon_path = prefix + icons[-1] + suffix
      tab_mult = needed_tabs - int(len(icon_path) / 8) + 1
      icon_path = "\t{}{}\\".format(icon_path, "\t" * tab_mult)
      print(icon_path, file=outfile)

  return max_len

if __name__ == "__main__":

  with open(color_mk, mode='w') as colorf, open(symbolic_mk, mode='w') as symbolicf:
    # Scalable icons.
    print("scalable_images = \\", file=colorf)
    print("scalable_images = \\", file=symbolicf)

    # Let's assume that scalable icons are the biggest list since it
    # should contain nearly all images. So we compute max_len once and
    # reuse this value on all lists.
    col_max_len = print_icons(list_dir, ['scalable.list'], None, "scalable/", ".svg", colorf)
    sym_max_len = print_icons(list_dir, ['scalable.list'], None, "scalable/", "-symbolic.svg", symbolicf)

    # 24x24 vector
    print("\nvector24_images = \\", file=colorf)
    print("\nvector24_images = \\", file=symbolicf)
    print_icons(list_dir, ['vector_24.list'], col_max_len, "24/", ".svg", colorf)
    print_icons(list_dir, ['vector_24.list'], sym_max_len, "24/", "-symbolic.svg", symbolicf)

    # 12x12 bitmap
    print("\nicons12_images = \\", file=colorf)
    print("\nicons12_images = \\", file=symbolicf)
    print_icons(list_dir, ['bitmap_12.list'], col_max_len, "12/", ".png", colorf)
    print_icons(list_dir, ['bitmap_12.list'], sym_max_len, "12/", "-symbolic.symbolic.png", symbolicf)

    # 16x16 bitmap
    print("\nicons16_images = \\", file=colorf)
    print("\nicons16_images = \\", file=symbolicf)
    print_icons(list_dir, ['bitmap_16.list'], col_max_len, "16/", ".png", colorf)
    print_icons(list_dir, ['bitmap_16.list'], sym_max_len, "16/", "-symbolic.symbolic.png", symbolicf)

    # 18x18 bitmap
    print("\nicons18_images = \\", file=colorf)
    print("\nicons18_images = \\", file=symbolicf)
    print_icons(list_dir, ['bitmap_18.list'], col_max_len, "18/", ".png", colorf)
    print_icons(list_dir, ['bitmap_18.list'], sym_max_len, "18/", "-symbolic.symbolic.png", symbolicf)

    # 20x20 bitmap
    print("\nicons20_images = \\", file=colorf)
    print("\nicons20_images = \\", file=symbolicf)
    print_icons(list_dir, ['bitmap_20.list'], col_max_len, "20/", ".png", colorf)
    print_icons(list_dir, ['bitmap_20.list'], sym_max_len, "20/", "-symbolic.symbolic.png", symbolicf)

    # 22x22 bitmap
    print("\nicons22_images = \\", file=colorf)
    print("\nicons22_images = \\", file=symbolicf)
    print_icons(list_dir, ['bitmap_22.list'], col_max_len, "22/", ".png", colorf)
    print_icons(list_dir, ['bitmap_22.list'], sym_max_len, "22/", "-symbolic.symbolic.png", symbolicf)

    # 24x24 bitmap
    print("\nicons24_images = \\", file=colorf)
    print("\nicons24_images = \\", file=symbolicf)
    print_icons(list_dir, ['bitmap_24.list'], col_max_len, "24/", ".png", colorf)
    print_icons(list_dir, ['bitmap_24.list'], sym_max_len, "24/", "-symbolic.symbolic.png", symbolicf)

    # 32x32 bitmap
    print("\nicons32_images = \\", file=colorf)
    print("\nicons32_images = \\", file=symbolicf)
    print_icons(list_dir, ['bitmap_32.list'], col_max_len, "32/", ".png", colorf)
    print_icons(list_dir, ['bitmap_32.list'], sym_max_len, "32/", "-symbolic.symbolic.png", symbolicf)

    # 48x48 bitmap
    print("\nicons48_images = \\", file=colorf)
    print("\nicons48_images = \\", file=symbolicf)
    print_icons(list_dir, ['bitmap_48.list'], col_max_len, "48/", ".png", colorf)
    print_icons(list_dir, ['bitmap_48.list'], sym_max_len, "48/", "-symbolic.symbolic.png", symbolicf)

    # 64x64 bitmap
    print("\nicons64_images = \\", file=colorf)
    print("\nicons64_images = \\", file=symbolicf)
    print_icons(list_dir, [ 'bitmap_64-always.list', 'bitmap_64.list'], col_max_len, "64/", ".png", colorf)
    print_icons(list_dir, [ 'bitmap_64-always.list' ], sym_max_len, "64/", ".png", symbolicf, endlist=False)
    print_icons(list_dir, [ 'bitmap_64.list'], sym_max_len, "64/", "-symbolic.symbolic.png", symbolicf)

    print("\nicons64_system_images = \\", file=colorf)
    print("\nicons64_system_images = \\", file=symbolicf)
    print_icons(list_dir, ['bitmap_64-system.list'], col_max_len, "64/", ".png", colorf)
    print_icons(list_dir, ['bitmap_64-system.list'], sym_max_len, "64/", "-symbolic.symbolic.png", symbolicf)

    # 96x96 bitmap
    print("\nicons96_images = \\", file=colorf)
    print("\nicons96_images = \\", file=symbolicf)
    print_icons(list_dir, ['bitmap_96.list'], col_max_len, "96/", ".png", colorf)
    print_icons(list_dir, ['bitmap_96.list'], sym_max_len, "96/", "-symbolic.symbolic.png", symbolicf)

    # 128x128 bitmap
    print("\nicons128_images = \\", file=colorf)
    print("\nicons128_images = \\", file=symbolicf)
    print_icons(list_dir, ['bitmap_128.list'], col_max_len, "128/", ".png", colorf)
    print_icons(list_dir, ['bitmap_128.list'], sym_max_len, "128/", "-symbolic.symbolic.png", symbolicf)

    # 192x192 bitmap
    print("\nicons192_images = \\", file=colorf)
    print("\nicons192_images = \\", file=symbolicf)
    print_icons(list_dir, ['bitmap_192.list'], col_max_len, "192/", ".png", colorf)
    print_icons(list_dir, ['bitmap_192.list'], sym_max_len, "192/", "-symbolic.symbolic.png", symbolicf)

    # 256x256 bitmap
    print("\nicons256_images = \\", file=colorf)
    print("\nicons256_images = \\", file=symbolicf)
    print_icons(list_dir, ['bitmap_256.list'], col_max_len, "256/", ".png", colorf)
    print_icons(list_dir, ['bitmap_256.list'], sym_max_len, "256/", "-symbolic.symbolic.png", symbolicf)

    print(file=colorf)
    print(file=symbolicf)
    eof = os.path.join(list_dir, "icon-list.mk.eof")
    with open(eof, mode='r') as f:
      for line in f:
        colorf.write(line)
        symbolicf.write(line)
