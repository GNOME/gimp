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

/* sRGB encoding/decoding */
int linear_to_sRGB(int c);
int sRGB_to_linear(int c);

/* YCoCg encoding */
static inline void
RGB_to_YCoCg (unsigned char *dst, int r, int g, int b)
{
  int y  = ((r +     (g << 1) + b) + 2) >> 2;
  int co = ((((r << 1) - (b << 1)) + 2) >> 2) + 128;
  int cg = (((-r +   (g << 1) - b) + 2) >> 2) + 128;

  dst[0] = 255;
  dst[1] = (cg > 255 ? 255 : (cg < 0 ? 0 : cg));
  dst[2] = (co > 255 ? 255 : (co < 0 ? 0 : co));
  dst[3] = (y  > 255 ? 255 : (y  < 0 ? 0 :  y));
}

/* other color conversions */

static inline int
rgb_to_luminance (int r, int g, int b)
{
  /* ITU-R BT.709 luma coefficients, scaled by 256 */
  return ((r * 54 + g * 182 + b * 20) + 128) >> 8;
}

static inline unsigned short
pack_r5g6b5 (int r, int g, int b)
{
  return (mul8bit(r, 31) << 11) |
         (mul8bit(g, 63) <<  5) |
         (mul8bit(b, 31)      );
}

static inline unsigned short
pack_rgba4 (int r, int g, int b, int a)
{
  return (mul8bit(a, 15) << 12) |
         (mul8bit(r, 15) <<  8) |
         (mul8bit(g, 15) <<  4) |
         (mul8bit(b, 15)      );
}

static inline unsigned short
pack_rgb5a1 (int r, int g, int b, int a)
{
  return (((a >> 7) & 0x01) << 15) |
         (mul8bit(r, 31)    << 10) |
         (mul8bit(g, 31)    <<  5) |
         (mul8bit(b, 31)         );
}

static inline unsigned char
pack_r3g3b2(int r, int g, int b)
{
  return (mul8bit(r, 7) << 5) |
         (mul8bit(g, 7) << 2) |
         (mul8bit(b, 3)     );
}

static inline unsigned int
pack_rgb10a2 (int r, int g, int b, int a)
{
  return ((unsigned int)((a >> 6) & 0x003) << 30) |
         ((unsigned int)((r << 2) & 0x3ff) << 20) |
         ((unsigned int)((g << 2) & 0x3ff) << 10) |
         ((unsigned int)((b << 2) & 0x3ff)      );
}

#endif /* __COLOR_H__ */
