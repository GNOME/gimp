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

#ifndef __IMATH_H__
#define __IMATH_H__

#ifndef MIN
# ifdef __GNUC__
#  define MIN(a, b)  ({typeof(a) _a=(a); typeof(b) _b=(b); _a < _b ? _a : _b;})
# else
#  define MIN(a, b)  ((a) < (b) ? (a) : (b))
# endif
#endif

#ifndef MAX
# ifdef __GNUC__
#  define MAX(a, b)  ({typeof(a) _a=(a); typeof(b) _b=(b); _a > _b ? _a : _b;})
# else
#  define MAX(a, b)  ((a) > (b) ? (a) : (b))
# endif
#endif

#define IS_POW2(x)     (!((x) & ((x) - 1)))
#define IS_MUL4(x)     (((x) & 3) == 0)

/* round integer x up to next multiple of 4 */
#define RND_MUL4(x)    ((x) + (4 - ((x) & 3)))

static inline int
mul8bit (int a,
         int b)
{
  int t = a * b + 128;

  return (t + (t >> 8)) >> 8;
}

static inline int
blerp (int a,
       int b,
       int x)
{
   return a + mul8bit(b - a, x);
}

static inline int
icerp (int a,
       int b,
       int c,
       int d,
       int x)
{
  int p = (d - c) - (a - b);
  int q = (a - b) - p;
  int r = c - a;

  return (x * (x * (x * p + (q << 7)) + (r << 14)) + (b << 21)) >> 21;
}

#endif /* __IMATH_H__ */
