/*
 * DDS GIMP plugin
 *
 * Copyright (C) 2004-2012 Shawn Kirst <skirst@gmail.com>,
 * with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __COLOR_H__
#define __COLOR_H__


#include "imath.h"


/* sRGB encoding & decoding */
gfloat linear_to_sRGB (gfloat c);
gfloat sRGB_to_linear (gfloat c);


/* YCoCg encoding */
static inline void
RGB_to_YCoCg (guchar *dst, gint r, gint g, gint b)
{
  gint y  = ((r +     (g << 1) + b) + 2) >> 2;
  gint co = ((((r << 1) - (b << 1)) + 2) >> 2) + 128;
  gint cg = (((-r +   (g << 1) - b) + 2) >> 2) + 128;

  dst[0] = 255;
  dst[1] = (cg > 255 ? 255 : (cg < 0 ? 0 : cg));
  dst[2] = (co > 255 ? 255 : (co < 0 ? 0 : co));
  dst[3] = (y  > 255 ? 255 : (y  < 0 ? 0 :  y));
}


/* Other color conversions */
static inline gint
rgb_to_luminance (gint r,
                  gint g,
                  gint b)
{
  /* ITU-R BT.709 luma coefficients, scaled by 256 */
  return ((r * 54 + g * 182 + b * 20) + 128) >> 8;
}

static inline gushort
pack_r5g6b5 (gint r,
             gint g,
             gint b)
{
  return (mul8bit (r, 31) << 11) |
         (mul8bit (g, 63) <<  5) |
         (mul8bit (b, 31)      );
}

static inline gushort
pack_rgba4 (gint r,
            gint g,
            gint b,
            gint a)
{
  return (mul8bit (a, 15) << 12) |
         (mul8bit (r, 15) <<  8) |
         (mul8bit (g, 15) <<  4) |
         (mul8bit (b, 15)      );
}

static inline gushort
pack_rgb5a1 (gint r,
             gint g,
             gint b,
             gint a)
{
  return (((a >> 7) & 0x01) << 15) |
         (mul8bit (r, 31)   << 10) |
         (mul8bit (g, 31)   <<  5) |
         (mul8bit (b, 31)        );
}

static inline guchar
pack_r3g3b2 (gint r,
             gint g,
             gint b)
{
  return (mul8bit (r, 7) << 5) |
         (mul8bit (g, 7) << 2) |
         (mul8bit (b, 3)     );
}

static inline guint
pack_rgb10a2 (gint r, gint g, gint b, gint a)
{
  return ((guint) ((a >> 6) & 0x003) << 30) |
         ((guint) ((b << 2) & 0x3ff) << 20) |
         ((guint) ((g << 2) & 0x3ff) << 10) |
         ((guint) ((r << 2) & 0x3ff)      );
}


#endif /* __COLOR_H__ */
