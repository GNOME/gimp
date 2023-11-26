/*
 * DDS GIMP plugin
 *
 * Copyright (C) 2004-2012 Shawn Kirst <skirst@gmail.com>,
 * with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 51 Franklin Street, Fifth Floor
 * Boston, MA 02110-1301, USA.
 */

#include <math.h>

#include <libgimp/gimp.h>

#include "color.h"


gfloat
linear_to_sRGB (gfloat c)
{
  gfloat v = (gfloat) c;

  if (v < 0.0f)
    v = 0.0f;
  else if (v > 1.0f)
    v = 1.0f;
  else if (v <= 0.0031308f)
    v = 12.92f * v;
  else
    v = 1.055f * powf (v, 0.41666f) - 0.055f;

  return v;
}

gfloat
sRGB_to_linear (gfloat c)
{
  gfloat v = (gfloat) c;

  if (v < 0.0f)
    v = 0.0f;
  else if (v > 1.0f)
    v = 1.0f;
  else if (v <= 0.04045f)
    v /= 12.92f;
  else
    v = powf ((v + 0.055f) / 1.055f, 2.4f);

  return v;
}
