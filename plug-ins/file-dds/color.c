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
#include "color.h"

int
linear_to_sRGB(int c)
{
  float v = (float)c / 255.0f;

  if(v < 0)
    v = 0;
  else if(v > 1)
    v = 1;
  else if(v <= 0.0031308f)
    v = 12.92f * v;
  else
    v = 1.055f * powf(v, 0.41666f) - 0.055f;

  return (int)floorf(255.0f * v + 0.5f);
}

int
sRGB_to_linear(int c)
{
  float v = (float)c / 255.0f;

  if(v < 0)
    v = 0;
  else if(v > 1)
    v = 1;
  else if(v <= 0.04045f)
    v /= 12.92f;
  else
    v = powf((v + 0.055f) / 1.055f, 2.4f);

  return (int)floorf(255.0f * v + 0.5f);
}
