#!/usr/bin/env python2

#   Gimp-Python - allows the writing of Gimp plugins in Python.
#   Copyright (C) 1997  James Henstridge <james@daa.com.au>
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Algorithms stolen from the whirl and pinch plugin distributed with Gimp,
# by Federico Mena Quintero and Scott Goehring
#
# This version does the same thing, except there is no preview, and it is
# written in python and is slower.

import math, struct
from gimpfu import *

class pixel_fetcher:
        def __init__(self, drawable):
                self.col = -1
                self.row = -1
                self.img_width = drawable.width
                self.img_height = drawable.height
                self.img_bpp = drawable.bpp
                self.img_has_alpha = drawable.has_alpha
                self.tile_width = gimp.tile_width()
                self.tile_height = gimp.tile_height()
                self.bg_colour = '\0\0\0\0'
                self.bounds = drawable.mask_bounds
                self.drawable = drawable
                self.tile = None
        def set_bg_colour(self, r, g, b, a):
                self.bg_colour = struct.pack('BBB', r,g,b)
                if self.img_has_alpha:
                        self.bg_colour = self.bg_colour + chr(a)
        def get_pixel(self, x, y):
                sel_x1, sel_y1, sel_x2, sel_y2 = self.bounds
                if x < sel_x1 or x >= sel_x2 or y < sel_y1 or y >= sel_y2:
                        return self.bg_colour
                col = x / self.tile_width
                coloff = x % self.tile_width
                row = y / self.tile_height
                rowoff = y % self.tile_height

                if col != self.col or row != self.row or self.tile is None:
                        self.tile = self.drawable.get_tile(False, row, col)
                        self.col = col
                        self.row = row
                return self.tile[coloff, rowoff]

class Dummy:
        pass

def whirl_pinch(image, drawable, whirl, pinch, radius):
        self = Dummy()
        self.width = drawable.width
        self.height = drawable.height
        self.bpp = drawable.bpp
        self.has_alpha = drawable.has_alpha
        self.bounds = drawable.mask_bounds
        self.sel_x1, self.sel_y1, self.sel_x2, self.sel_y2 = \
                     drawable.mask_bounds
        self.sel_w = self.sel_x2 - self.sel_x1
        self.sel_h = self.sel_y2 - self.sel_y1
        self.cen_x = (self.sel_x1 + self.sel_x2 - 1) / 2.0
        self.cen_y = (self.sel_y1 + self.sel_y2 - 1) / 2.0
        xhsiz = (self.sel_w - 1) / 2.0
        yhsiz = (self.sel_h - 1) / 2.0

        if xhsiz < yhsiz:
                self.scale_x = yhsiz / xhsiz
                self.scale_y = 1.0
        elif xhsiz > yhsiz:
                self.scale_x = 1.0
                self.scale_y = xhsiz / yhsiz
        else:
                self.scale_x = 1.0
                self.scale_y = 1.0

        self.radius = max(xhsiz, yhsiz);

        if not drawable.is_rgb and not drawable.is_grey:
                return

        gimp.tile_cache_ntiles(2 * (1 + self.width / gimp.tile_width()))

        whirl = whirl * math.pi / 180
        dest_rgn = drawable.get_pixel_rgn(self.sel_x1, self.sel_y1,
                                          self.sel_w, self.sel_h, True, True)
        pft = pixel_fetcher(drawable)
        pfb = pixel_fetcher(drawable)

        bg_colour = gimp.get_background()

        pft.set_bg_colour(bg_colour[0], bg_colour[1], bg_colour[2], 0)
        pfb.set_bg_colour(bg_colour[0], bg_colour[1], bg_colour[2], 0)

        progress = 0
        max_progress = self.sel_w * self.sel_h

        gimp.progress_init("Whirling and pinching")

        self.radius2 = self.radius * self.radius * radius
        pixel = ['', '', '', '']
        values = [0,0,0,0]

        for row in range(self.sel_y1, (self.sel_y1+self.sel_y2)/2+1):
                top_p = ''
                bot_p = ''
                for col in range(self.sel_x1, self.sel_x2):
                        q, cx, cy = calc_undistorted_coords(self, col,
                                                            row, whirl, pinch,
                                                            radius)
                        if q:
                                if cx >= 0: ix = int(cx)
                                else:       ix = -(int(-cx) + 1)
                                if cy >= 0: iy = int(cy)
                                else:       iy = -(int(-cy) + 1)
                                pixel[0] = pft.get_pixel(ix, iy)
                                pixel[1] = pft.get_pixel(ix+1, iy)
                                pixel[2] = pft.get_pixel(ix, iy+1)
                                pixel[3] = pft.get_pixel(ix+1, iy+1)
                                for i in range(self.bpp):
                                        values[0] = ord(pixel[0][i])
                                        values[1] = ord(pixel[1][i])
                                        values[2] = ord(pixel[2][i])
                                        values[3] = ord(pixel[3][i])
                                        top_p = top_p + bilinear(cx,cy, values)
                                cx = self.cen_x + (self.cen_x - cx)
                                cy = self.cen_y + (self.cen_y - cy)
                                if cx >= 0: ix = int(cx)
                                else:       ix = -(int(-cx) + 1)
                                if cy >= 0: iy = int(cy)
                                else:       iy = -(int(-cy) + 1)
                                pixel[0] = pfb.get_pixel(ix, iy)
                                pixel[1] = pfb.get_pixel(ix+1, iy)
                                pixel[2] = pfb.get_pixel(ix, iy+1)
                                pixel[3] = pfb.get_pixel(ix+1, iy+1)
                                tmp = ''
                                for i in range(self.bpp):
                                        values[0] = ord(pixel[0][i])
                                        values[1] = ord(pixel[1][i])
                                        values[2] = ord(pixel[2][i])
                                        values[3] = ord(pixel[3][i])
                                        tmp = tmp + bilinear(cx,cy, values)
                                bot_p = tmp + bot_p
                        else:
                                top_p = top_p + pft.get_pixel(col, row)
                                bot_p = pfb.get_pixel((self.sel_x2 - 1) -
                                        (col - self.sel_x1), (self.sel_y2-1) -
                                        (row - self.sel_y1)) + bot_p

                dest_rgn[self.sel_x1:self.sel_x2, row] = top_p
                dest_rgn[self.sel_x1:self.sel_x2, (self.sel_y2 - 1)
                         - (row - self.sel_y1)] = bot_p

                progress = progress + self.sel_w * 2
                gimp.progress_update(float(progress) / max_progress)

        drawable.flush()
        drawable.merge_shadow(True)
        drawable.update(self.sel_x1,self.sel_y1,self.sel_w,self.sel_h)

def calc_undistorted_coords(self, wx, wy, whirl, pinch, radius):
        dx = (wx - self.cen_x) * self.scale_x
        dy = (wy - self.cen_y) * self.scale_y
        d = dx * dx + dy * dy
        inside = d < self.radius2

        if inside:
                dist = math.sqrt(d / radius) / self.radius
                if (d == 0.0):
                        factor = 1.0
                else:
                        factor = math.pow(math.sin(math.pi / 2 * dist),
                                          -pinch)
                dx = dx * factor
                dy = dy * factor
                factor = 1 - dist
                ang = whirl * factor * factor
                sina = math.sin(ang)
                cosa = math.cos(ang)
                x = (cosa * dx - sina * dy) / self.scale_x + self.cen_x
                y = (sina * dx + cosa * dy) / self.scale_y + self.cen_y
        else:
                x = wx
                y = wy
        return inside, float(x), float(y)

def bilinear(x, y, values):
        x = x % 1.0
        y = y % 1.0
        m0 = values[0] + x * (values[1] - values[0])
        m1 = values[2] + x * (values[3] - values[2])
        return chr(int(m0 + y * (m1 - m0)))


register(
        "python-fu-whirl-pinch",
        "Distorts an image by whirling and pinching",
        "Distorts an image by whirling and pinching",
        "James Henstridge (translated from C plugin)",
        "James Henstridge",
        "1997-1999",
        "_Whirl and Pinch...",
        "RGB*, GRAY*",
        [
            (PF_IMAGE, "image", "Input image", None),
            (PF_DRAWABLE, "drawable", "Input drawable", None),
            (PF_SLIDER, "whirl", "Whirl angle", 90, (-360, 360, 1)),
            (PF_FLOAT, "pinch", "Pinch amount", 0),
            (PF_FLOAT, "radius", "radius", 1)
        ],
        [],
        whirl_pinch, menu="<Image>/Filters/Distorts")

main()
